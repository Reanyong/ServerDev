// Echo WebSocket

#include <boost//beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <iostream>

using tcp = boost::asio::ip::tcp;
namespace websocket = boost::beast::websocket;

int main()
{
	try {


		boost::asio::io_context ioc;

		tcp::acceptor acceptor(ioc, { tcp::v4(), 8080 });
		for (;;)
		{
			// This will receive the new connection
			tcp::socket socket(ioc);
			// Block until we get a connection
			acceptor.accept(socket);
			// Launch the session, transferring ownership of the socket
			std::thread{ std::bind(&do_session, std::move(socket)) }.detach();
		}
	}
	catch (std::exception const& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
}

