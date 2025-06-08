// models/ApiResponse.cpp
#include "ApiResponse.h"
#include <ctime>
#include <iomanip>
#include <sstream>

ApiResponse::ApiResponse(http::status status) : status_(status) {
    response_data_["status"] = static_cast<int>(status);
}

ApiResponse& ApiResponse::setData(const nlohmann::json& data) {
    response_data_["data"] = data;
    return *this;
}

ApiResponse& ApiResponse::setMessage(const std::string& message) {
    response_data_["message"] = message;
    return *this;
}

ApiResponse& ApiResponse::setError(const std::string& error) {
    response_data_["error"] = error;
    return *this;
}

ApiResponse& ApiResponse::setTimestamp(std::time_t timestamp) {
    response_data_["timestamp"] = timestamp;
    return *this;
}

ApiResponse& ApiResponse::setPath(const std::string& path) {
    response_data_["path"] = path;
    return *this;
}

ApiResponse& ApiResponse::setMethod(const std::string& method) {
    response_data_["method"] = method;
    return *this;
}

http::response<http::string_body> ApiResponse::toHttpResponse(unsigned version) const {
    http::response<http::string_body> response{ status_, version };

    std::string body = toJsonString();
    response.body() = body;
    response.content_length(body.size());

    addCommonHeaders(response);
    return response;
}

std::string ApiResponse::toJsonString() const {
    return response_data_.dump(2); // 들여쓰기 포함
}

void ApiResponse::addCommonHeaders(http::response<http::string_body>& response) const {
    response.set(http::field::server, "API Server/1.0");
    response.set(http::field::content_type, "application/json; charset=utf-8");
    response.set(http::field::access_control_allow_origin, "*");
    response.set(http::field::access_control_allow_methods, "GET, POST, PUT, DELETE, OPTIONS");
    response.set(http::field::access_control_allow_headers, "Content-Type, Authorization");
}
