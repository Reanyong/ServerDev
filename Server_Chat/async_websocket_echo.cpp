#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <Windows.h>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = net::ip::tcp;               // from <boost/asio/ip/tcp.hpp>
using namespace std;                    // for convenience

wstring utf8_to_wstring(const string& str)
{
	if (str.empty()) return wstring();

	// 버퍼 크기 계산
	int size_need = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);

	// 버퍼 할당
	wstring result(size_need, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &result[0], size_need);

	return result;
}

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
			wcerr << L"[Session] WebSocket 연결 실패: " << utf8_to_wstring(ec.message()) << endl;
			return;
		}

		wcout << L"[Session] WebSocket 연결 성공" << endl;

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
				wcout << L"[Session] WebSocket closed." << endl;
			}
			else {
				wcerr << L"[Error] Read: " << utf8_to_wstring(ec.message()) << endl;
			}
			return;
		}

		// 수신한 메시지 처리
		string message = beast::buffers_to_string(buffer_.data());
		wcout << L"[Recv] " << utf8_to_wstring(message) << endl;

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
			wcerr << L"[Error] Write: " << utf8_to_wstring(ec.message()) << endl;
			return;
		}

		// 버퍼 비우기
		buffer_.consume(buffer_.size());

		// 다음 메시지 읽기
		do_read();
	}
};

int main() {
	try {
		SetConsoleOutputCP(CP_UTF8);

		// io_context - Boost.Asio의 핵심 클래스
		net::io_context ioc;

		// TCP 수신기 기본 포트
		tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), 8080));

		// 클라이언트 연결 수락
		tcp::socket socket(ioc);
		acceptor.accept(socket);

		// 세션 생성 및 시작
		make_shared<Session>(move(socket))->start();

		// 비동기 작업 실행
		ioc.run();
	}
	catch (const exception& e) {
		cerr << e.what() << endl;
	}

	return 0;
}