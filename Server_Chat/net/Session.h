// Session.h
#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <mutex>
#include <queue>
#include <atomic>
#include "../chat/ChatRoom.h"

// 전방 선언
class ChatRoom;
class WebSocketServer;

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
public:
    // 생성자/소멸자
    explicit Session(tcp::socket socket, std::shared_ptr<ChatRoom> chat_room, std::weak_ptr<WebSocketServer> ws_server = {});
    ~Session();

    // 세션 제어
    void start();
    void close();
    bool is_open() const;

    // 메시지 송신
    void send(const std::string& message);

    // 사용자 정보 접근자
    const std::string& getNickname() const;
    void setNickname(const std::string& nickname);
    const std::string& getDbUserId() const;
    int getUserId() const { return user_id_; }

    // 핑/퐁 타이머 제어
    void startPingTimer();

    // DB 관련 함수
    bool registerUser();
    bool endSession();

    // 세션 상태 확인
    bool isClosed() const { return closed_; }
    bool isClosing() const { return closing_; }

private:
    std::shared_ptr<ChatRoom> chat_room_;
    std::weak_ptr<WebSocketServer> ws_server_;  // WebSocketServer에 대한 약한 참조
    // WebSocket 관련
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;

    // 동기화 관련
    std::mutex write_mutex_;
    std::mutex read_mutex_;
    std::atomic<bool> reading_{ false };
    std::atomic<bool> writing_{ false };
    std::atomic<bool> closing_{ false };
    std::atomic<bool> closed_{ false };

    // 사용자 정보
    std::string nickname_;
    int user_id_;
    std::string db_user_id_;

    // 메시지 큐
    std::queue<std::string> write_queue_;

    // 내부 처리 함수들
    void queue_message(const std::string& message);
    void do_write();
    void on_write(beast::error_code ec, size_t bytes_transferred, std::shared_ptr<std::string> message_ptr);
    void on_accept(beast::error_code ec);
    void handle_disconnect(beast::error_code ec);
    void do_read();
    void on_read(beast::error_code ec, size_t bytes_transferred);

    // 명령어 처리
    bool processCommand(const std::string& message);
    void handleNicknameChange(const std::string& new_nickname);
    void handleWhisperCommand(const std::string& target_and_message);
};
