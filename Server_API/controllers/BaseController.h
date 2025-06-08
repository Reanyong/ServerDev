// controllers/BaseController.h
#pragma once

#include "../models/ApiResponse.h"
#include "../models/HttpMethod.h"
#include <boost/beast/http.hpp>
#include <string>
#include <memory>

namespace http = boost::beast::http;

class BaseController {
public:
    virtual ~BaseController() = default;

    // 가상 메소드들
    virtual ApiResponse handleGet(const std::string& path, const std::string& query = "") {
        return createMethodNotAllowed("GET");
    }

    virtual ApiResponse handlePost(const std::string& path, const std::string& body = "") {
        return createMethodNotAllowed("POST");
    }

    virtual ApiResponse handlePut(const std::string& path, const std::string& body = "") {
        return createMethodNotAllowed("PUT");
    }

    virtual ApiResponse handleDelete(const std::string& path, const std::string& query = "") {
        return createMethodNotAllowed("DELETE");
    }

    // 요청 처리 메인 메소드
    ApiResponse handleRequest(HttpMethod method, const std::string& path,
        const std::string& body = "", const std::string& query = "");

protected:
    // 유틸리티 메소드들
    ApiResponse createSuccess(const nlohmann::json& data = {}, const std::string& message = "");
    ApiResponse createError(http::status status, const std::string& error);
    ApiResponse createNotFound(const std::string& path);
    ApiResponse createMethodNotAllowed(const std::string& method);
    ApiResponse createBadRequest(const std::string& error);
};
