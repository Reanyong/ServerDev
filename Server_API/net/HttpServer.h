// HttpServer.h
#pragma once

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include "../config/ApiConfig.h"

namespace net = boost::asio;
using tcp = net::ip::tcp;

class HttpSession;

class HttpServer {
public:
    explicit HttpServer(const ApiConfig& config);
    ~HttpServer();

    bool start();
    void stop();
    bool isRunning() const { return running_; }

    // 서버 정보
    std::string getAddress() const;
    uint16_t getPort() const { return port_; }

private:
    ApiConfig config_;
    std::string address_;
    uint16_t port_;
    int thread_count_;

    std::unique_ptr<net::io_context> ioc_;
    std::unique_ptr<tcp::acceptor> acceptor_;
    std::vector<std::thread> threads_;

    std::atomic<bool> running_{ false };
    std::atomic<bool> stopping_{ false };

    void do_accept();
    void wait_for_threads();
};
