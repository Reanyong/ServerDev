// routes/ApiRouter.cpp
#include "ApiRouter.h"
#include "../controllers/HealthController.h"
#include "../controllers/TestController.h"

ApiRouter::ApiRouter() {
    // 기본 라우트 등록
    registerRoute("/health", std::make_shared<HealthController>());
    registerRoute("/api/test", std::make_shared<TestController>());
}

void ApiRouter::registerRoute(const std::string& path, std::shared_ptr<BaseController> controller) {
    std::string normalized_path = normalizePath(path);
    routes_[normalized_path] = controller;
}

ApiResponse ApiRouter::handleRequest(HttpMethod method, const std::string& path,
    const std::string& body, const std::string& query) {
    std::string normalized_path = normalizePath(path);

    auto it = routes_.find(normalized_path);
    if (it != routes_.end()) {
        ApiResponse response = it->second->handleRequest(method, path, body, query);
        return response.setPath(path).setMethod(HttpMethodUtils::toString(method));
    }

    return createNotFoundResponse(path);
}

std::vector<std::string> ApiRouter::getRegisteredRoutes() const {
    std::vector<std::string> route_list;
    for (const auto& [path, controller] : routes_) {
        route_list.push_back(path);
    }
    return route_list;
}

std::string ApiRouter::normalizePath(const std::string& path) const {
    if (path.empty() || path == "/") {
        return "/";
    }

    std::string normalized = path;

    // 끝의 슬래시 제거
    if (normalized.length() > 1 && normalized.back() == '/') {
        normalized.pop_back();
    }

    return normalized;
}

ApiResponse ApiRouter::createNotFoundResponse(const std::string& path) const {
    return ApiResponse(http::status::not_found)
        .setError("Not Found")
        .setMessage("The requested endpoint '" + path + "' was not found")
        .setPath(path)
        .setTimestamp();
}
