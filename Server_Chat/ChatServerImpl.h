// ChatServerImpl.h
#pragma once

#include "IChatServer.h"
#include "config/ServerConfig.h"
#include <memory>
#include <string>
#include <atomic>

// 전방 선언
class ServerConfig;
class WebSocketServer;
class ChatRoom;
class ChatRepository;
class DatabaseManager;
class Session;

class ChatServerImpl : public IChatServer {
public:
    // 생성자/소멸자
    explicit ChatServerImpl(const ServerConfig& config);
    ~ChatServerImpl() override;

    // IChatServer 인터페이스 구현
    bool Initialize() override;
    bool Start() override;
    void Stop() override;
    bool IsRunning() const override;

    size_t GetConnectionCount() const override;
    std::string GetServerAddress() const override;
    uint16_t GetServerPort() const override;

    void SetLogCallback(LogCallback callback) override;
    std::string GetServerStats() const override;

private:
    // 구성 요소
    std::unique_ptr<WebSocketServer> ws_server_;
    std::unique_ptr<ChatRoom> chat_room_;
    std::shared_ptr<DatabaseManager> db_manager_;
    std::unique_ptr<ChatRepository> chat_repository_;

    // 설정 정보
    ServerConfig config_;

    // 상태 정보
    std::atomic<bool> initialized_{ false };
    LogCallback log_callback_;

    // 내부 처리 함수
    void onClientConnected(std::shared_ptr<Session> session);
    void onClientDisconnected(std::shared_ptr<Session> session);
    void log(const std::string& message);
    bool initializeDatabase();
};
