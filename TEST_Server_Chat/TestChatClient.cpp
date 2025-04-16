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
    int connect_delay_ms = 50;      // 연결 간 지연
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
        config.client_count = 100;
        config.message_count = 50;
        config.connect_delay_ms = 5;       // 연결 지연 최소화
        config.message_delay_min_ms = 50;  // 메시지 지연 축소
        config.message_delay_max_ms = 150; // 메시지 지연 축소

        // 신호 핸들러 설정
        setup_signal_handlers();

        // 난수 발생기 초기화
        srand(static_cast<unsigned int>(time(nullptr)));

        cout << "WebSocket 채팅 서버 부하 테스트 시작" << endl;
        cout << "대상 서버: " << config.host << ":" << config.port << endl;
        cout << "클라이언트 수: " << config.client_count << endl;
        cout << "클라이언트당 메시지 수: " << config.message_count << endl;

        // IO 컨텍스트 생성 - 스레드 수만큼
        int thread_count = thread::hardware_concurrency();
        net::io_context ioc{ thread_count };

        // 작업 완료 신호용 카운터
        net::executor_work_guard<net::io_context::executor_type> work_guard =
            net::make_work_guard(ioc);

        // 워커 스레드 생성 - IO 컨텍스트 실행
        vector<thread> ioc_threads;
        for (int i = 0; i < thread_count; ++i) {
            ioc_threads.emplace_back([&ioc]() {
                ioc.run();
                });
        }

        // 클라이언트 생성
        vector<shared_ptr<Client>> clients;
        clients.reserve(config.client_count);
        g_clients = clients; // 전역 참조 설정

        // RAII 클라이언트 관리자
        ClientManager manager(clients);

        // 클라이언트 생성 및 비동기 연결
        cout << "클라이언트 연결 중..." << endl;
        atomic<int> connected_count{ 0 };
        atomic<int> connection_attempts{ 0 };

        // 진행 상황 출력 함수 미리 정의
        function<void(const beast::error_code&)> update_progress;

        // 진행 상황 출력용 타이머
        net::steady_timer progress_timer(ioc, chrono::milliseconds(100));

        // 진행 상황 업데이트 함수 정의
        update_progress = [&progress_timer, &connection_attempts, &config, &connected_count, &update_progress](const beast::error_code& ec) {
            if (ec) return;

            cout << "\r연결 진행: " << connection_attempts << "/"
                << config.client_count << " 시도, "
                << connected_count << " 연결됨" << flush;

            if (connection_attempts < config.client_count) {
                progress_timer.expires_after(chrono::milliseconds(100));
                progress_timer.async_wait(update_progress);
            }
            };

        // 타이머 시작
        progress_timer.async_wait(update_progress);

        // 동시 연결 제한 (과부하 방지)
        const int MAX_CONCURRENT_CONNECTS = 5;
        atomic<int> active_connects{ 0 };
        mutex connect_mutex;
        condition_variable connect_cv;

        // 연결 완료 신호용
        mutex completion_mutex;
        condition_variable completion_cv;

        // 모든 클라이언트 생성
        for (int i = 0; i < config.client_count; ++i) {
            clients.push_back(make_shared<Client>(ioc, i, config));
        }

        // 병렬 연결 시작
        for (int i = 0; i < config.client_count; ++i) {
            // 연결 수 제한 (과부하 방지)
            {
                unique_lock<mutex> lock(connect_mutex);
                connect_cv.wait(lock, [&]() {
                    return active_connects < MAX_CONCURRENT_CONNECTS;
                    });
                active_connects++;
            }

            // 비동기 연결 시작
            net::post(ioc, [&, i]() {
                try {
                    if (clients[i]->connect()) {
                        connected_count++;
                    }
                }
                catch (const std::exception& e) {
                    cerr << "Client " << i << " connection error: " << e.what() << endl;
                }

                connection_attempts++;

                // 연결 슬롯 해제
                active_connects--;
                connect_cv.notify_one();

                // 마지막 클라이언트 연결 완료시 통지
                if (connection_attempts >= config.client_count) {
                    unique_lock<mutex> lock(completion_mutex);
                    completion_cv.notify_all();
                }
                });

            // 과부하 방지용 아주 짧은 지연 (선택적)
            this_thread::sleep_for(chrono::milliseconds(1));
        }

        // 모든 연결 완료 대기
        {
            unique_lock<mutex> lock(completion_mutex);
            completion_cv.wait(lock, [&]() {
                return connection_attempts >= config.client_count;
                });
        }

        cout << "\n" << connected_count << "/" << config.client_count << " 클라이언트 연결됨" << endl;

        if (connected_count == 0) {
            cerr << "연결된 클라이언트가 없음, 테스트 중단" << endl;
            work_guard.reset();
            for (auto& t : ioc_threads) {
                if (t.joinable()) t.join();
            }
            return EXIT_FAILURE;
        }

        // 테스트 통계 초기화
        TestStats stats;

        // 테스트 시작 시간 기록
        auto start_time = chrono::high_resolution_clock::now();

        // 연결된 클라이언트만 필터링
        vector<shared_ptr<Client>> connected_clients;
        for (auto& client : clients) {
            if (client->is_connected()) {
                connected_clients.push_back(client);
            }
        }

        // 스레드 풀 크기 설정
        int test_thread_count = min(100, static_cast<int>(thread::hardware_concurrency() * 2));
        int clients_per_thread = (connected_clients.size() + test_thread_count - 1) / test_thread_count;

        cout << "부하 테스트 시작 (스레드 " << test_thread_count << "개 사용)" << endl;

        // 테스트 진행 상황 출력용 타이머 및 함수
        atomic<int> completed_clients{ 0 };
        net::steady_timer test_progress_timer(ioc, chrono::milliseconds(500));

        // 테스트 진행 상태 업데이트 함수
        function<void(const beast::error_code&)> update_test_progress;

        update_test_progress = [&test_progress_timer, &completed_clients, &connected_clients, &update_test_progress](const beast::error_code& ec) {
            if (ec) return;

            cout << "\r테스트 진행: " << completed_clients << "/"
                << connected_clients.size() << " 클라이언트 완료" << flush;

            if (completed_clients < connected_clients.size()) {
                test_progress_timer.expires_after(chrono::milliseconds(500));
                test_progress_timer.async_wait(update_test_progress);
            }
            };

        // 진행 상황 타이머 시작
        test_progress_timer.async_wait(update_test_progress);

        // 테스트 완료 신호용
        mutex test_completion_mutex;
        condition_variable test_completion_cv;

        // 클라이언트 메시지 전송 테스트 시작
        for (size_t i = 0; i < connected_clients.size(); ++i) {
            net::post(ioc, [&, i]() {
                try {
                    connected_clients[i]->send_messages(stats);
                }
                catch (const std::exception& e) {
                    cerr << "Client test error: " << e.what() << endl;
                }

                completed_clients++;

                // 마지막 클라이언트 테스트 완료시 통지
                if (completed_clients >= connected_clients.size()) {
                    unique_lock<mutex> lock(test_completion_mutex);
                    test_completion_cv.notify_all();
                }
                });
        }

        // 모든 테스트 완료 대기
        {
            unique_lock<mutex> lock(test_completion_mutex);
            test_completion_cv.wait(lock, [&]() {
                return completed_clients >= connected_clients.size();
                });
        }

        // 테스트 종료 시간 기록
        auto end_time = chrono::high_resolution_clock::now();
        auto elapsed = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count() / 1000.0;

        cout << "\n테스트 완료!" << endl;

        // 결과 출력
        stats.print_summary(elapsed);

        // IO 컨텍스트 작업 중단 및 스레드 정리
        work_guard.reset();

        for (auto& t : ioc_threads) {
            if (t.joinable()) t.join();
        }

        // 정리 (ClientManager에서 자동 처리)
        cout << "테스트 완료" << endl;

        return EXIT_SUCCESS;
    }
    catch (exception const& e) {
        cerr << "오류: " << e.what() << endl;
        return EXIT_FAILURE;
    }
}
