// Echo WebSocket

#include <boost//beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <iostream>

namespace beast = boost::beast;
namespace websocket = boost::beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

int main()
{
	try
	{
		net::io_context ioc;
		tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), 8080));

		std::cout << "[ChatServer] Listening on port 8080..." << std::endl;

		tcp::socket socket(ioc);
		acceptor.accept(socket);

		websocket::stream<tcp::socket> ws(std::move(socket));
		ws.accept();

		std::cout << "[Session] WebSocket connected." << std::endl;

		for (;;) {
			beast::flat_buffer buffer;
			ws.read(buffer);

			std::string message = beast::buffers_to_string(buffer.data());
			std::cout << "[Recv] " << message << std::endl;

			ws.text(ws.got_text());
			ws.write(net::buffer(message));
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
	}

	return 0;
}

