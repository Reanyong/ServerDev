// models/ApiResponse.h
#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <boost/beast/http.hpp>

namespace http = boost::beast::http;

class ApiResponse {
public:
    ApiResponse(http::status status = http::status::ok);

    // 데이터 설정
    ApiResponse& setData(const nlohmann::json& data);
    ApiResponse& setMessage(const std::string& message);
    ApiResponse& setError(const std::string& error);

    // 메타데이터 설정
    ApiResponse& setTimestamp(std::time_t timestamp = std::time(nullptr));
    ApiResponse& setPath(const std::string& path);
    ApiResponse& setMethod(const std::string& method);

    // HTTP 응답 생성
    http::response<http::string_body> toHttpResponse(unsigned version) const;

    // JSON 문자열 반환
    std::string toJsonString() const;

private:
    http::status status_;
    nlohmann::json response_data_;

    // 공통 헤더 추가
    void addCommonHeaders(http::response<http::string_body>& response) const;
};
