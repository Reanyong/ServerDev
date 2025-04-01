// Echo WebSocket
// **#**

#include <boost//beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <locale>
#include <Windows.h>
#include <io.h>
#include <fcntl.h>

namespace beast = boost::beast;
namespace websocket = boost::beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

int main()
{
	try
	{
		SetConsoleOutputCP(CP_UTF8);

		_setmode(_fileno(stdout), _O_U8TEXT);

		//std::locale::global(std::locale(""));
		//std::wcout.imbue(std::locale());

		net::io_context ioc;
		tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), 8080));

		std::wcout << L"[ChatServer] Listening on port 8080..." << std::endl;

		tcp::socket socket(ioc);
		acceptor.accept(socket);

		websocket::stream<tcp::socket> ws(std::move(socket));

		ws.text(true);

		ws.accept();

		std::wcout << L"[Session] WebSocket connected." << std::endl;

		for (;;) {
			beast::flat_buffer buffer;
			ws.read(buffer);

			std::string message = beast::buffers_to_string(buffer.data());

			int wchars_num = MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, NULL, 0);
			std::wstring wmessage(wchars_num, 0);
			MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, &wmessage[0], wchars_num);

			std::wcout << L"[Recv] " << wmessage.c_str() << std::endl;

			ws.text(ws.got_text());
			ws.write(net::buffer(message));
		}
	}
	catch (const std::exception& e)
	{
		int wchars_num = MultiByteToWideChar(CP_UTF8, 0, e.what(), -1, NULL, 0);
		std::wstring werror(wchars_num, 0);
		MultiByteToWideChar(CP_UTF8, 0, e.what(), -1, &werror[0], wchars_num);

		std::cerr << "Error: " << e.what() << std::endl;
	}

	return 0;
}
// **#**