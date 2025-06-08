// routes/ApiRouter.h
#pragma once

#include "../models/ApiResponse.h"
#include "../models/HttpMethod.h"
#include "../controllers/BaseController.h"
#include <memory>
#include <unordered_map>
#include <string>

class ApiRouter {
public:
    ApiRouter();
    ~ApiRouter() = default;

    // 라우트 등록
    void registerRoute(const std::string& path, std::shared_ptr<BaseController> controller);

    // 요청 처리
    ApiResponse handleRequest(HttpMethod method, const std::string& path,
        const std::string& body = "", const std::string& query = "");

    // 등록된 라우트 목록
    std::vector<std::string> getRegisteredRoutes() const;

private:
    std::unordered_map<std::string, std::shared_ptr<BaseController>> routes_;

    // 경로 정규화
    std::string normalizePath(const std::string& path) const;

    // 404 응답 생성
    ApiResponse createNotFoundResponse(const std::string& path) const;
};
