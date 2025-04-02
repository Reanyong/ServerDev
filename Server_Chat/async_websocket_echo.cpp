// **#**
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <Windows.h>
#include <io.h>
#include <fcntl.h>

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
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwWritten = 0;
    WriteConsoleW(hConsole, message.c_str(), (DWORD)message.length(), &dwWritten, NULL);
    WriteConsoleW(hConsole, L"\r\n", 2, &dwWritten, NULL);
}

// 콘솔에 에러 메시지 출력
void ConsoleErr(const wstring& message)
{
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

        ConsoleOut(L"[Session] WebSocket 연결 성공");

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
                ConsoleOut(L"[Session] 클라이언트 연결 종료");
            }
            else {
                ConsoleErr(L"[Error] Read: " + utf8_to_wstring(ec.message()));
            }
            return;
        }

        // 수신한 메시지 처리
        string message = beast::buffers_to_string(buffer_.data());
        ConsoleOut(L"[Recv] " + utf8_to_wstring(message));

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
            ConsoleErr(L"[Error] Write: " + utf8_to_wstring(ec.message()));
            return;
        }

        // 버퍼 비우기
        buffer_.consume(buffer_.size());

        // 다음 메시지 읽기
        do_read();
    }
};

// 비동기 연결 수락을 처리
void handle_new_connection(const shared_ptr<tcp::acceptor>& acceptor,
    const shared_ptr<net::io_context>& ioc) {
    acceptor->async_accept(
        [acceptor, ioc](beast::error_code ec, tcp::socket socket) {
            if (!ec) {
                ConsoleOut(L"[Server] 새 클라이언트 연결 수락됨");
                make_shared<Session>(move(socket))->start();
            }
            else {
                ConsoleErr(L"[Error] Accept: " + utf8_to_wstring(ec.message()));
            }

            handle_new_connection(acceptor, ioc);
        });
}

int main() {
    try {
        // 콘솔 설정
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);

        // 시작 메시지
        ConsoleOut(L"Server Start!! - Port 8080");

        // IO 컨텍스트 생성
        auto ioc = make_shared<net::io_context>();

        // TCP 수신기 생성
        auto acceptor = make_shared<tcp::acceptor>(*ioc, tcp::endpoint(tcp::v4(), 8080));

        ConsoleOut(L"클라이언트 연결 대기 중...");

        // 첫 클라이언트 연결 수락 (동기식)
        tcp::socket socket(*ioc);
        acceptor->accept(socket);

        ConsoleOut(L"첫 클라이언트 연결됨");

        // 세션 생성 및 시작
        make_shared<Session>(move(socket))->start();

        // 다음 연결들은 비동기로 처리
        handle_new_connection(acceptor, ioc);

        // 비동기 작업 실행
        ioc->run();
    }
    catch (const exception& e) {
        cerr << "Exception: " << e.what() << endl;
    }

    return 0;
}
// **#**