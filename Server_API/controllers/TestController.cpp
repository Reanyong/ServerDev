// controllers/TestController.cpp
#include "TestController.h"

ApiResponse TestController::handleGet(const std::string& path, const std::string& query) {
    nlohmann::json test_data;
    test_data["message"] = "Hello from TestController!";
    test_data["path"] = path;
    test_data["query"] = query;
    test_data["method"] = "GET";

    return createSuccess(test_data, "GET request processed successfully");
}

ApiResponse TestController::handlePost(const std::string& path, const std::string& body) {
    nlohmann::json test_data;
    test_data["message"] = "POST request received";
    test_data["path"] = path;
    test_data["body"] = body;
    test_data["method"] = "POST";

    return createSuccess(test_data, "POST request processed successfully");
}

ApiResponse TestController::handlePut(const std::string& path, const std::string& body) {
    nlohmann::json test_data;
    test_data["message"] = "PUT request received";
    test_data["path"] = path;
    test_data["body"] = body;
    test_data["method"] = "PUT";

    return createSuccess(test_data, "PUT request processed successfully");
}

ApiResponse TestController::handleDelete(const std::string& path, const std::string& query) {
    nlohmann::json test_data;
    test_data["message"] = "DELETE request received";
    test_data["path"] = path;
    test_data["query"] = query;
    test_data["method"] = "DELETE";

    return createSuccess(test_data, "DELETE request processed successfully");
}
