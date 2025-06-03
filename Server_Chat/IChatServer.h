// IChatServer.h
#pragma once

#include <string>
#include <functional>
#include <memory>

// 전방 선언
class ServerConfig;

// 채팅 서버 인터페이스
class IChatServer {
public:
    // 가상 소멸자
    virtual ~IChatServer() = default;

    // 서버 제어 메소드
    virtual bool Initialize() = 0;
    virtual bool Start() = 0;
    virtual void Stop() = 0;
    virtual bool IsRunning() const = 0;

    // 상태 조회 메소드
    virtual size_t GetConnectionCount() const = 0;
    virtual std::string GetServerAddress() const = 0;
    virtual uint16_t GetServerPort() const = 0;

    // 콜백 설정
    using LogCallback = std::function<void(const std::string&)>;
    virtual void SetLogCallback(LogCallback callback) = 0;

    // 서버 통계 정보
    virtual std::string GetServerStats() const = 0;
};

// 팩토리 함수 선언
std::unique_ptr<IChatServer> CreateChatServer(const ServerConfig& config);
