// HttpSession.h
#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class HttpSession : public std::enable_shared_from_this<HttpSession> {
public:
    explicit HttpSession(tcp::socket socket);
    ~HttpSession() = default;

    void start();

private:
    tcp::socket socket_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> request_;

    void do_read();
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void handle_request();
    void do_write(http::response<http::string_body> response);
    void on_write(beast::error_code ec, std::size_t bytes_transferred, bool close);

    // 라우팅 처리
    http::response<http::string_body> create_response(
        http::status status,
        const std::string& content_type,
        const std::string& body);
};
