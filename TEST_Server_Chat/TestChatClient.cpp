#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include <csignal>
#include <iomanip>
#include <Windows.h>
#include <locale>
#include <codecvt>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;
using namespace std;

std::wstring utf8_to_wstring(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_need = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
    std::wstring result(size_need, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &result[0], size_need);
    return result;
}

// 와이드 문자열(wstring)을 UTF-8 문자열로 변환
std::string wstring_to_utf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_need = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string result(size_need, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &result[0], size_need, nullptr, nullptr);
    return result;
}

// 테스트 통계 수집용 구조체
struct TestStats {
    atomic<size_t> total_messages{ 0 };
    atomic<size_t> successful_messages{ 0 };
    atomic<size_t> failed_messages{ 0 };
    chrono::milliseconds total_response_time{ 0 };
    chrono::milliseconds min_response_time{ chrono::hours(1) };
    chrono::milliseconds max_response_time{ 0 };
    mutable mutex stats_mutex;

    void update_response_time(const chrono::milliseconds& time) {
        lock_guard<mutex> lock(stats_mutex);
        total_response_time += time;
        if (time < min_response_time) min_response_time = time;
        if (time > max_response_time) max_response_time = time;
    }

    void print_summary(double elapsed_seconds) const {
        cout << "---------- 테스트 결과 요약 ----------" << endl;
        cout << "총 테스트 시간: " << fixed << setprecision(2) << elapsed_seconds << " 초" << endl;
        cout << "총 메시지 수: " << total_messages << endl;

        size_t success_count = successful_messages.load();
        size_t total_count = total_messages.load();
        double success_rate = (total_count > 0) ? (success_count * 100.0 / total_count) : 0;

        cout << "성공한 메시지: " << success_count
            << " (" << success_rate << "%)" << endl;
        cout << "실패한 메시지: " << failed_messages << endl;

        lock_guard<mutex> lock(stats_mutex);
        if (success_count > 0) {
            auto avg_response = chrono::milliseconds(total_response_time.count() / success_count);
            cout << "평균 응답 시간: " << avg_response.count() << " ms" << endl;
            cout << "최소 응답 시간: " << min_response_time.count() << " ms" << endl;
            cout << "최대 응답 시간: " << max_response_time.count() << " ms" << endl;
        }

        cout << "처리량: " << (success_count / elapsed_seconds) << " messages/sec" << endl;
        cout << "---------------------------------------" << endl;
    }
};

// 테스트 설정 구조체
struct TestConfig {
    string host = "localhost";
    unsigned short port = 8080;
    int client_count = 100;
    int message_count = 50;
    int connect_delay_ms = 20;      // 연결 간 지연
    int message_delay_min_ms = 100; // 최소 메시지 지연
    int message_delay_max_ms = 300; // 최대 메시지 지연
    int read_timeout_ms = 5000;     // 5초 읽기 타임아웃
};

// 전역 클라이언트 목록 (신호 핸들러용)
vector<shared_ptr<class Client>> g_clients;

// 클라이언트 세션 클래스
class Client : public enable_shared_from_this<Client> {

private:
    string nickname_;
public:
    // 생성자에서 설정
    Client(net::io_context& ioc, unsigned int id, const TestConfig& config)
        : id_(id),
        ws_(net::make_strand(ioc)),
        ping_timer_(ws_.get_executor()),
        config_(config),
        buffer_(8192) // 8KB로 버퍼 크기 제한
    {
    }

    // 서버에 연결
    bool connect() {
        try {
            tcp::resolver resolver(ws_.get_executor());
            auto results = resolver.resolve(config_.host, to_string(config_.port));

            beast::error_code ec;

            // 수정된 부분: next_layer()를 통해 기본 tcp 스트림에 접근
            beast::get_lowest_layer(ws_).connect(results, ec);

            if (ec) {
                cerr << "Client " << id_ << " connect error: " << ec.message() << endl;
                return false;
            }

            // WebSocket 핸드셰이크
            ws_.handshake(config_.host, "/", ec);

            if (ec) {
                cerr << "Client " << id_ << " handshake error: " << ec.message() << endl;
                return false;
            }

            // ping/pong 핸들러 설정
            setup_pong_handler();

            // 닉네임 설정 부분 제거
            // 기존 닉네임(TestClient+ID)을 그대로 사용하므로 별도 요청 안 함

            // 닉네임 저장만 수행 (서버에 전송하지 않음)
            nickname_  = "TestClient" + to_string(id_);

            // 환영 메시지 대기 제거 - 서버가 특정 응답을 보낼 때까지 대기하지 않음

            // ping 타이머 시작
            start_ping_timer();

            return true;
        }
        catch (exception const& e) {
            cerr << "Client " << id_ << " connect exception: " << e.what() << endl;
            return false;
        }
    }

    // 메시지 전송
    void send_messages(TestStats& stats) {
        for (int i = 0; i < config_.message_count; ++i) {
            stats.total_messages++;

            try {
                // 메시지 생성 - 메시지 형식을 단순화(서버 부담 감소)
                string msg = "Msg" + to_string(i) + " from " + to_string(id_);

                // 응답 시간 측정 시작
                auto start_time = chrono::high_resolution_clock::now();

                // 메시지 전송
                beast::error_code ec;
                ws_.write(net::buffer(msg), ec);

                if (ec) {
                    cerr << "Client " << id_ << " write error: " << ec.message() << endl;
                    stats.failed_messages++;
                    continue;
                }

                // 응답 읽기
                buffer_.consume(buffer_.size());
                read_with_timeout(chrono::milliseconds(config_.read_timeout_ms), ec);

                if (ec) {
                    cerr << "Client " << id_ << " read error: " << ec.message() << endl;
                    stats.failed_messages++;
                    continue;
                }

                // 응답 시간 측정 종료
                auto end_time = chrono::high_resolution_clock::now();
                auto response_time = chrono::duration_cast<chrono::milliseconds>(
                    end_time - start_time);

                // 통계 업데이트
                stats.successful_messages++;
                stats.update_response_time(response_time);

                // 메시지 간 지연 - 부하 테스트 시 지연 시간 줄이기
                int delay = config_.message_delay_min_ms +
                    rand() % (config_.message_delay_max_ms - config_.message_delay_min_ms + 1);

                // 부하 테스트에서는 지연 시간을 짧게 설정
                this_thread::sleep_for(chrono::milliseconds(delay));
            }
            catch (exception const& e) {
                cerr << "Client " << id_ << " exception: " << e.what() << endl;
                stats.failed_messages++;
            }
        }
    }

    // 타임아웃 설정된 읽기
    bool read_with_timeout(chrono::milliseconds timeout, beast::error_code& ec) {
        // 동기 읽기로 단순화
        ws_.read(buffer_, ec);
        return !ec;
    }

    // 연결 종료
    void close() {
        beast::error_code ec;
        ws_.close(websocket::close_code::normal, ec);

        if (ec && ec != beast::errc::not_connected) {
            cerr << "Client " << id_ << " close error: " << ec.message() << endl;
        }
    }

    // 연결 상태 확인
    bool is_connected() const {
        return ws_.is_open();
    }

private:
    // ping/pong 핸들러 설정
    void setup_pong_handler() {
        ws_.control_callback(
            [this](websocket::frame_type kind, beast::string_view payload) {
                if (kind == websocket::frame_type::pong) {
                    last_pong_time_ = chrono::steady_clock::now();
                }
            });
    }

    // 주기적 핑 타이머 시작
    void start_ping_timer() {
        ping_timer_.expires_after(chrono::seconds(30));
        ping_timer_.async_wait(
            [this](beast::error_code ec) {
                if (ec || !ws_.is_open()) {
                    return;
                }

                beast::error_code ping_ec;
                ws_.ping("", ping_ec);

                if (ping_ec) {
                    cerr << "Client " << id_ << " ping error: " << ping_ec.message() << endl;
                }
                else {
                    start_ping_timer(); // 다음 ping 예약
                }
            });
    }

private:
    unsigned int id_;
    websocket::stream<beast::tcp_stream> ws_;
    net::steady_timer ping_timer_;
    TestConfig config_;
    beast::flat_buffer buffer_;
    chrono::steady_clock::time_point last_pong_time_ = chrono::steady_clock::now();
};

// RAII 방식의 클라이언트 관리
class ClientManager {
public:
    ClientManager(vector<shared_ptr<Client>>& clients)
        : clients_(clients) {
    }

    ~ClientManager() {
        close_all();
    }

    void close_all() {
        for (auto& client : clients_) {
            try {
                if (client && client->is_connected()) {
                    client->close();
                }
            }
            catch (...) {
                // 무시
            }
        }
        cout << "모든 클라이언트 연결 정리 완료" << endl;
    }

private:
    vector<shared_ptr<Client>>& clients_;
};

// 신호 핸들러 설정
void setup_signal_handlers() {
    signal(SIGINT, [](int signal) {
        cout << "인터럽트 수신, 정리 중..." << endl;
        for (auto& client : g_clients) {
            try {
                if (client) client->close();
            }
            catch (...) {
                // 무시
            }
        }
        exit(0);
        });
}

int main() {
    try {
#ifdef _WIN32
        // 한글 출력을 위한 콘솔 코드 페이지 설정
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);

        // 로케일 설정 추가
        std::locale::global(std::locale(".UTF-8"));
        std::wcout.imbue(std::locale());
        std::wcin.imbue(std::locale());
#endif

        // 설정 초기화
        TestConfig config;
        config.client_count = 20;
        config.message_count = 50;

        // 신호 핸들러 설정
        setup_signal_handlers();

        // 난수 발생기 초기화
        srand(static_cast<unsigned int>(time(nullptr)));

        cout << "WebSocket 채팅 서버 부하 테스트 시작" << endl;
        cout << "대상 서버: " << config.host << ":" << config.port << endl;
        cout << "클라이언트 수: " << config.client_count << endl;
        cout << "클라이언트당 메시지 수: " << config.message_count << endl;

        // IO 컨텍스트 생성
        net::io_context ioc;

        // 클라이언트 생성
        vector<shared_ptr<Client>> clients;
        clients.reserve(config.client_count);
        g_clients = clients; // 전역 참조 설정

        // RAII 클라이언트 관리자
        ClientManager manager(clients);

        // 클라이언트 생성
        for (int i = 0; i < config.client_count; ++i) {
            clients.push_back(make_shared<Client>(ioc, i, config));
        }

        // 모든 클라이언트 연결
        cout << "클라이언트 연결 중..." << endl;
        int connected_count = 0;

        for (auto& client : clients) {
            if (client->connect()) {
                connected_count++;
            }

            // 연결 간 지연
            this_thread::sleep_for(chrono::milliseconds(config.connect_delay_ms));
        }

        cout << connected_count << "/" << config.client_count << " 클라이언트 연결됨" << endl;

        if (connected_count == 0) {
            cerr << "연결된 클라이언트가 없음, 테스트 중단" << endl;
            return EXIT_FAILURE;
        }

        // 테스트 통계 초기화
        TestStats stats;

        // 테스트 시작 시간 기록
        auto start_time = chrono::high_resolution_clock::now();

        // 스레드 풀 크기 설정
        int thread_count = min(100, static_cast<int>(thread::hardware_concurrency() * 2));
        int clients_per_thread = (connected_count + thread_count - 1) / thread_count;

        cout << "부하 테스트 시작 (스레드 " << thread_count << "개 사용)" << endl;

        // 스레드 생성 및 메시지 전송
        vector<thread> threads;
        threads.reserve(thread_count);

        for (int t = 0; t < thread_count; ++t) {
            int start_idx = t * clients_per_thread;
            if (start_idx >= connected_count) break;

            int end_idx = min(start_idx + clients_per_thread, connected_count);

            threads.emplace_back([&clients, &stats, start_idx, end_idx]() {
                for (int i = start_idx; i < end_idx; ++i) {
                    if (clients[i]->is_connected()) {
                        clients[i]->send_messages(stats);
                    }
                }
                });
        }

        // 모든 스레드 종료 대기
        for (auto& t : threads) {
            if (t.joinable()) t.join();
        }

        // 테스트 종료 시간 기록
        auto end_time = chrono::high_resolution_clock::now();
        auto elapsed = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count() / 1000.0;

        // 결과 출력
        stats.print_summary(elapsed);

        // 정리 (ClientManager에서 자동 처리)
        cout << "테스트 완료" << endl;

        return EXIT_SUCCESS;
    }
    catch (exception const& e) {
        cerr << "오류: " << e.what() << endl;
        return EXIT_FAILURE;
    }
}
