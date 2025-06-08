// controllers/HealthController.cpp
#include "HealthController.h"
#include <thread>

ApiResponse HealthController::handleGet(const std::string& path, const std::string& query) {
    nlohmann::json health_data;
    health_data["server"] = "API Server";
    health_data["version"] = "1.0.0";
    health_data["status"] = "healthy";
    health_data["uptime"] = "계산 필요"; // 실제로는 서버 시작 시간 계산
    health_data["thread_count"] = std::thread::hardware_concurrency();

    return createSuccess(health_data, "Server is running normally");
}
