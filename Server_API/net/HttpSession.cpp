// HttpSession.cpp
#include "HttpSession.h"
#include "../../Server_Chat/utils/ConsoleHelper.h"
#include "../routes/ApiRouter.h"
#include "../models/HttpMethod.h"
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
    HttpMethod method = HttpMethodUtils::fromBeastMethod(request_.method());
    std::string body = request_.body();

    ConsoleHelper::Out("[HTTP] " + HttpMethodUtils::toString(method) + " " + target);

    // 정적 라우터 인스턴스
    static ApiRouter router;

    // 라우터를 통해 요청 처리
    ApiResponse api_response = router.handleRequest(method, target, body);

    // HTTP 응답으로 변환
    http::response<http::string_body> response = api_response.toHttpResponse(request_.version());

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
