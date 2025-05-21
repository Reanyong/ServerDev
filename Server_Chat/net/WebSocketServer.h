// WebSocketServer.h
#pragma once

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <memory>
#include <functional>
#include "../config/ServerConfig.h"

namespace beast = boost::beast;
namespace net = boost::asio;
using tcp = net::ip::tcp;

// 전방 선언
class Session;

class WebSocketServer {
public:
    // 생성자/소멸자
    WebSocketServer(const ServerConfig& config);
    ~WebSocketServer();

    // 서버 시작/중지
    bool start();
    void stop();
    bool isRunning() const { return running_; }

    // 이벤트 콜백 설정
    using ConnectionCallback = std::function<void(std::shared_ptr<Session>)>;
    void setOnClientConnected(ConnectionCallback callback) { onClientConnected_ = callback; }
    void setOnClientDisconnected(ConnectionCallback callback) { onClientDisconnected_ = callback; }

    // 서버 상태 정보
    size_t getConnectionCount() const { return connection_count_; }
    std::string getServerAddress() const;
    uint16_t getServerPort() const { return port_; }

private:
    // 설정 정보
    std::string address_;
    uint16_t port_;
    int thread_count_;

    // Boost.Asio 관련
    std::unique_ptr<net::io_context> ioc_;
    std::unique_ptr<tcp::acceptor> acceptor_;
    std::vector<std::thread> threads_;
    std::vector<net::strand<net::io_context::executor_type>> strands_;

    // 서버 상태
    std::atomic<bool> running_{ false };
    std::atomic<bool> stopping_{ false };
    std::atomic<size_t> connection_count_{ 0 };
    std::atomic<int> next_strand_index_{ 0 };

    // 콜백 함수
    ConnectionCallback onClientConnected_;
    ConnectionCallback onClientDisconnected_;

    // 내부 처리 함수
    void do_accept();
    void wait_for_threads();
};
