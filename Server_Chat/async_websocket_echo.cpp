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

	// ���� ũ�� ���
	int size_need = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);

	// ���� �Ҵ�
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
	// ������ �޾� ���� ����
	explicit Session(tcp::socket socket)
		: ws_(move(socket))
	{
	}

	void start()
	{
		// �񵿱������� ������ �ڵ����ũ ����
		ws_.async_accept(
			beast::bind_front_handler(
				&Session::on_accept,
				shared_from_this()));
	}

private:
	// ������ ���� �� �ݹ�
	void on_accept(beast::error_code ec)
	{
		if (ec) {
			wcerr << L"[Session] WebSocket ���� ����: " << utf8_to_wstring(ec.message()) << endl;
			return;
		}

		wcout << L"[Session] WebSocket ���� ����" << endl;

		// �񵿱������� ������ ���� ���
		do_read();
	}

	// �񵿱������� �޽��� �б�
	void do_read() {
		ws_.async_read(
			buffer_,
			beast::bind_front_handler(
				&Session::on_read,
				shared_from_this()));
	}

	// �޽��� �б� �Ϸ� �� ȣ��Ǵ� �ݹ�
	void on_read(beast::error_code ec, size_t bytes_transferred) {
		boost::ignore_unused(bytes_transferred);

		// ���� ó��
		if (ec) {
			if (ec == websocket::error::closed) {
				wcout << L"[Session] WebSocket closed." << endl;
			}
			else {
				wcerr << L"[Error] Read: " << utf8_to_wstring(ec.message()) << endl;
			}
			return;
		}

		// ������ �޽��� ó��
		string message = beast::buffers_to_string(buffer_.data());
		wcout << L"[Recv] " << utf8_to_wstring(message) << endl;

		// ���� ���� (�񵿱������� �޽��� ����)
		ws_.text(ws_.got_text());
		ws_.async_write(
			buffer_.data(),
			beast::bind_front_handler(
				&Session::on_write,
				shared_from_this()));
	}

	// �޽��� ���� �Ϸ� �� ȣ��Ǵ� �ݹ�
	void on_write(beast::error_code ec, size_t bytes_transferred) {
		boost::ignore_unused(bytes_transferred);

		if (ec) {
			wcerr << L"[Error] Write: " << utf8_to_wstring(ec.message()) << endl;
			return;
		}

		// ���� ����
		buffer_.consume(buffer_.size());

		// ���� �޽��� �б�
		do_read();
	}
};

int main() {
	try {
		SetConsoleOutputCP(CP_UTF8);

		// io_context - Boost.Asio�� �ٽ� Ŭ����
		net::io_context ioc;

		// TCP ���ű� �⺻ ��Ʈ
		tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), 8080));

		// Ŭ���̾�Ʈ ���� ����
		tcp::socket socket(ioc);
		acceptor.accept(socket);

		// ���� ���� �� ����
		make_shared<Session>(move(socket))->start();

		// �񵿱� �۾� ����
		ioc.run();
	}
	catch (const exception& e) {
		cerr << e.what() << endl;
	}

	return 0;
}