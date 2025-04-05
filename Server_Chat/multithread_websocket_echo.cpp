// **#**
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <Windows.h>
#include <io.h>
#include <fcntl.h>
#include <string>
#include <thread>
#include <vector>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;
using namespace std;

// UTF-8 문자열을 와이드 문자열로 변환
wstring utf8_to_wstring(const string& str)
{
    if (str.empty()) return wstring();
    int size_need = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
    wstring result(size_need, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &result[0], size_need);
    return result;
}

// 콘솔에 메시지 출력
void ConsoleOut(const wstring& message)
{
    static mutex console_mutex;
	lock_guard<mutex> lock(console_mutex);  // 스레드 안전성을 위해 뮤텍스 사용

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwWritten = 0;
    WriteConsoleW(hConsole, message.c_str(), (DWORD)message.length(), &dwWritten, NULL);
    WriteConsoleW(hConsole, L"\r\n", 2, &dwWritten, NULL);
}

// 콘솔에 에러 메시지 출력
void ConsoleErr(const wstring& message)
{
	static mutex console_mutex;
	lock_guard<mutex> lock(console_mutex);  // 스레드 안전성을 위해 뮤텍스 사용

    HANDLE hConsole = GetStdHandle(STD_ERROR_HANDLE);
    DWORD dwWritten = 0;
    WriteConsoleW(hConsole, message.c_str(), (DWORD)message.length(), &dwWritten, NULL);
    WriteConsoleW(hConsole, L"\r\n", 2, &dwWritten, NULL);
}

// 세션 클래스
class Session : public enable_shared_from_this<Session>
{
private:
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;
	mutex write_mutex_;  // 쓰기 작업, 동시 사용 방지를 위한 뮤텍스

public:
    // 소켓을 받아 세션 생성
    explicit Session(tcp::socket socket)
        : ws_(move(socket))
    {
    }

    void start()
    {
        // 비동기적으로 웹소켓 핸드셰이크 수행
        ws_.async_accept(
            beast::bind_front_handler(
                &Session::on_accept,
                shared_from_this()));
    }

private:
    // 웹소켓 연결 후 콜백
    void on_accept(beast::error_code ec)
    {
        if (ec) {
            ConsoleErr(L"[Session] WebSocket 연결 실패: " + utf8_to_wstring(ec.message()));
            return;
        }

        ConsoleOut(L"[Session] WebSocket 연결 성공 [Thread ID: " + 
            to_wstring(hash<thread::id>{}(this_thread::get_id())) + L"]");

        // 비동기적으로 데이터 수신 대기
        do_read();
    }

    // 비동기적으로 메시지 읽기
    void do_read() {
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &Session::on_read,
                shared_from_this()));
    }

    // 메시지 읽기 완료 후 호출되는 콜백
    void on_read(beast::error_code ec, size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        // 오류 처리
        if (ec) {
            if (ec == websocket::error::closed) {
                ConsoleOut(L"[Session] 클라이언트 연결 종료 [Thread ID: " + 
                    to_wstring(hash<thread::id>{}(this_thread::get_id())) + L"]");
            }
            else {
                ConsoleErr(L"[Error] Read: " + utf8_to_wstring(ec.message()) + 
                    L"[Thread ID: " + to_wstring(hash<thread::id>{}(this_thread::get_id())) + L"]");
            }
            return;
        }

        // 수신한 메시지 처리
        string message = beast::buffers_to_string(buffer_.data());
        ConsoleOut(L"[Recv] " + utf8_to_wstring(message));

		lock_guard<mutex> lock(write_mutex_);  // 쓰기 작업 동기화

        // 에코 응답 (비동기적으로 메시지 전송)
        ws_.text(ws_.got_text());
        ws_.async_write(
            buffer_.data(),
            beast::bind_front_handler(
                &Session::on_write,
                shared_from_this()));
    }

    // 메시지 쓰기 완료 후 호출되는 콜백
    void on_write(beast::error_code ec, size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        if (ec) {
            ConsoleErr(L"[Error] Write: " + utf8_to_wstring(ec.message()) + 
                L"[Thread ID: " + to_wstring(hash<thread::id>{}(this_thread::get_id())) + L"]");
            return;
        }

        // 버퍼 비우기
        buffer_.consume(buffer_.size());

        // 다음 메시지 읽기
        do_read();
    }
};

class Server {
private:
	net::io_context& ioc_;
	tcp::acceptor acceptor_;
	vector<thread> threads_;

public:
    Server(net::io_context& ioc, const tcp::endpoint& endpoint, int thread_count)
		: ioc_(ioc),
		acceptor_(ioc, endpoint)
    {
        // 멀티스레드 생성
		// CPU 코어 수에 따라 스레드 생성 -> 최소 thread_count 만큼 생성
        int num_threads = max(thread_count, 1);
		ConsoleOut(L"[Server] " + to_wstring(num_threads) + L"개의 스레드로 서버 시작");

        do_accept();
    }

    // 서버 스레드 시작
	void run() {
        // 하드웨어 코어 수 또는 지정된 값으로 최적의 스레드 수 결정
		int num_threads = thread::hardware_concurrency();

        // 스레드 생성 및 실행
        threads_.reserve(num_threads);

		for (int i = 0; i < num_threads; ++i) {
            threads_.emplace_back([this, i] {
                ConsoleOut(L"[Thread] 작업 스레드 #" + to_wstring(i) + L" 시작 [ID: " +
                    to_wstring(hash<thread::id>{}(this_thread::get_id())) + L"]");

                // 각 스레드가 io_context에서 작업을 처리
                this->ioc_.run();

                ConsoleOut(L"[Thread] 작업 스레드 #" + to_wstring(i) + L" 종료");
            });
		}
		// 모든 스레드가 종료될 때까지 대기
		for (auto& thread : threads_) {
			if (thread.joinable()) {
				thread.join();
			}
		}
	}

private:
	// 클라이언트 연결 수락
    void do_accept() {
		acceptor_.async_accept(
			[this](beast::error_code ec, tcp::socket socket) {
				if (!ec) {
					ConsoleOut(L"[Server] 클라이언트 연결됨 [Thread ID: " +
						to_wstring(hash<thread::id>{}(this_thread::get_id())) + L"]");

					// 세션 생성 및 시작
					make_shared<Session>(move(socket))->start();
				}
				else {
					ConsoleErr(L"[Error] Accept: " + utf8_to_wstring(ec.message()));
				}
				// 다음 클라이언트 연결 대기
				do_accept();
			});
    }
};

int main() {
    try {
        // 콘솔 설정
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);

        // 시작 메시지
        ConsoleOut(L"MultiThread Websocket Start!! - Port 8080");

		int thread_count = thread::hardware_concurrency(); // 하드웨어 코어 수 정보 출력
        ConsoleOut(L"시스템 CPU 코어/스레드 수: " + to_wstring(thread_count));

        // IO 컨텍스트 생성 - 각 스레드가 작업을 할당받을 컨텍스트
        net::io_context ioc{ thread_count };

        // 서버 인스턴스 생성 및 시작
        Server server(ioc, tcp::endpoint(tcp::v4(), 8080), thread_count);

        // 서버 실행 (블로킹)
        server.run();
    }
    catch (const exception& e) {
        cerr << "Exception: " << e.what() << endl;
        ConsoleErr(L"예외 발생: " + utf8_to_wstring(e.what()));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
// **#**
