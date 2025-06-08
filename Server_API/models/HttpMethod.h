// models/HttpMethod.h
#pragma once

#include <string>
#include <boost/beast/http.hpp>

namespace http = boost::beast::http;

enum class HttpMethod {
    Get,
    Post,
    Put,
    Delete,
    Patch,
    Options,
    Head,
    Unknown
};

class HttpMethodUtils {
public:
    static HttpMethod fromBeastMethod(http::verb method);
    static std::string toString(HttpMethod method);
    static http::verb toBeastMethod(HttpMethod method);
};

// 인라인 구현
inline HttpMethod HttpMethodUtils::fromBeastMethod(http::verb method) {
    switch (method) {
    case http::verb::get:
        return HttpMethod::Get;
    case http::verb::post:
        return HttpMethod::Post;
    case http::verb::put:
        return HttpMethod::Put;
    case http::verb::delete_:
        return HttpMethod::Delete;
    case http::verb::patch:
        return HttpMethod::Patch;
    case http::verb::options:
        return HttpMethod::Options;
    case http::verb::head:
        return HttpMethod::Head;
    default:
        return HttpMethod::Unknown;
    }
}

inline std::string HttpMethodUtils::toString(HttpMethod method) {
    switch (method) {
    case HttpMethod::Get: return "GET";
    case HttpMethod::Post: return "POST";
    case HttpMethod::Put: return "PUT";
    case HttpMethod::Delete: return "DELETE";
    case HttpMethod::Patch: return "PATCH";
    case HttpMethod::Options: return "OPTIONS";
    case HttpMethod::Head: return "HEAD";
    default: return "UNKNOWN";
    }
}

inline http::verb HttpMethodUtils::toBeastMethod(HttpMethod method) {
    switch (method) {
    case HttpMethod::Get: return http::verb::get;
    case HttpMethod::Post: return http::verb::post;
    case HttpMethod::Put: return http::verb::put;
    case HttpMethod::Delete: return http::verb::delete_;
    case HttpMethod::Patch: return http::verb::patch;
    case HttpMethod::Options: return http::verb::options;
    case HttpMethod::Head: return http::verb::head;
    default: return http::verb::unknown;
    }
}
