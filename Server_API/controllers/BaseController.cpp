// controllers/BaseController.cpp
#include "BaseController.h"

ApiResponse BaseController::handleRequest(HttpMethod method, const std::string& path,
    const std::string& body, const std::string& query) {
    try {
        switch (method) {
        case HttpMethod::Get:        // GET → Get로 변경
            return handleGet(path, query);
        case HttpMethod::Post:       // POST → Post로 변경
            return handlePost(path, body);
        case HttpMethod::Put:        // PUT → Put로 변경
            return handlePut(path, body);
        case HttpMethod::Delete:     // DELETE → Delete로 변경
            return handleDelete(path, query);
        default:
            return createMethodNotAllowed(HttpMethodUtils::toString(method));
        }
    }
    catch (const std::exception& e) {
        return createError(http::status::internal_server_error,
            "Internal server error: " + std::string(e.what()));
    }
}

ApiResponse BaseController::createSuccess(const nlohmann::json& data, const std::string& message) {
    ApiResponse response(http::status::ok);
    if (!data.empty()) {
        response.setData(data);
    }
    if (!message.empty()) {
        response.setMessage(message);
    }
    return response.setTimestamp();
}

ApiResponse BaseController::createError(http::status status, const std::string& error) {
    return ApiResponse(status).setError(error).setTimestamp();
}

ApiResponse BaseController::createNotFound(const std::string& path) {
    return ApiResponse(http::status::not_found)
        .setError("Not Found")
        .setPath(path)
        .setTimestamp();
}

ApiResponse BaseController::createMethodNotAllowed(const std::string& method) {
    return ApiResponse(http::status::method_not_allowed)
        .setError("Method Not Allowed")
        .setMessage("HTTP method '" + method + "' is not supported for this endpoint")
        .setTimestamp();
}

ApiResponse BaseController::createBadRequest(const std::string& error) {
    return ApiResponse(http::status::bad_request)
        .setError("Bad Request")
        .setMessage(error)
        .setTimestamp();
}
