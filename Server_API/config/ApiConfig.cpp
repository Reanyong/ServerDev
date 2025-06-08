// ApiConfig.cpp
#include "ApiConfig.h"
#include <thread>
#include <sstream>

ApiConfig::ApiConfig() {
    // 기본값은 헤더에서 설정
    if (thread_count_ <= 0) {
        thread_count_ = std::thread::hardware_concurrency();
    }
}

std::string ApiConfig::toString() const {
    std::stringstream ss;
    ss << "=== API 서버 설정 ===" << std::endl
        << "주소: " << address_ << std::endl
        << "포트: " << port_ << std::endl
        << "스레드 수: " << thread_count_ << std::endl;
    return ss.str();
}
