// ApiConfig.h
#pragma once

#include <string>

class ApiConfig {
public:
    ApiConfig();
    ~ApiConfig() = default;

    // 서버 설정
    const std::string& getAddress() const { return address_; }
    uint16_t getPort() const { return port_; }
    int getThreadCount() const { return thread_count_; }

    // 설정 변경
    void setAddress(const std::string& address) { address_ = address; }
    void setPort(uint16_t port) { port_ = port; }
    void setThreadCount(int count) { thread_count_ = count; }

    // 설정 정보 문자열
    std::string toString() const;

private:
    std::string address_ = "0.0.0.0";    // 모든 인터페이스
    uint16_t port_ = 8081;               // API 서버 포트 (채팅서버와 다름)
    int thread_count_ = 0;               // 0이면 하드웨어 스레드 수
};
