// HttpSession.cpp
#include "HttpSession.h"
#include "../../Server_Chat/utils/ConsoleHelper.h"
#include <nlohmann/json.hpp>

HttpSession::HttpSession(tcp::socket socket)
    : socket_(std::move(socket)) {
}

void HttpSession::start() {
    do_read();
}

void HttpSession::do_read() {
    auto self = shared_from_this();

    http::async_read(socket_, buffer_, request_,
        [self](beast::error_code ec, std::size_t bytes_transferred) {
            self->on_read(ec, bytes_transferred);
        });
}

void HttpSession::on_read(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (ec == http::error::end_of_stream) {
        socket_.shutdown(tcp::socket::shutdown_send, ec);
        return;
    }

    if (ec) {
        ConsoleHelper::Error("[HTTP] 읽기 오류: " + ec.message());
        return;
    }

    handle_request();
}

void HttpSession::handle_request() {
    std::string target = std::string(request_.target());
    std::string method = std::string(request_.method_string());

    ConsoleHelper::Out("[HTTP] " + method + " " + target);

    http::response<http::string_body> response;

    // 간단한 라우팅
    if (target == "/health") {
        nlohmann::json json_response;
        json_response["status"] = "ok";
        json_response["server"] = "API Server";
        json_response["version"] = "1.0.0";

        response = create_response(
            http::status::ok,
            "application/json",
            json_response.dump());
    }
    else if (target == "/api/test") {
        nlohmann::json json_response;
        json_response["message"] = "Hello API Server!";
        json_response["timestamp"] = std::time(nullptr);
        json_response["method"] = method;

        response = create_response(
            http::status::ok,
            "application/json",
            json_response.dump());
    }
    else {
        // 404 Not Found
        nlohmann::json json_response;
        json_response["error"] = "Not Found";
        json_response["path"] = target;

        response = create_response(
            http::status::not_found,
            "application/json",
            json_response.dump());
    }

    do_write(std::move(response));
}

http::response<http::string_body> HttpSession::create_response(
    http::status status,
    const std::string& content_type,
    const std::string& body) {

    http::response<http::string_body> response{ status, request_.version() };
    response.set(http::field::server, "API Server/1.0");
    response.set(http::field::content_type, content_type);
    response.set(http::field::access_control_allow_origin, "*"); // CORS
    response.content_length(body.size());
    response.body() = body;
    return response;
}

void HttpSession::do_write(http::response<http::string_body> response) {
    auto self = shared_from_this();
    auto sp = std::make_shared<http::response<http::string_body>>(std::move(response));

    http::async_write(socket_, *sp,
        [self, sp](beast::error_code ec, std::size_t bytes_transferred) {
            self->on_write(ec, bytes_transferred, sp->need_eof());
        });
}

void HttpSession::on_write(beast::error_code ec, std::size_t bytes_transferred, bool close) {
    boost::ignore_unused(bytes_transferred);

    if (ec) {
        ConsoleHelper::Error("[HTTP] 쓰기 오류: " + ec.message());
        return;
    }

    if (close) {
        socket_.shutdown(tcp::socket::shutdown_send, ec);
        return;
    }

    // 다음 요청 읽기
    do_read();
}
