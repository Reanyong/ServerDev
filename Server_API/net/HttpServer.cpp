// HttpServer.cpp
#include "HttpServer.h"
#include "HttpSession.h"
#include "../../Server_Chat/utils/ConsoleHelper.h"
#include <algorithm>

HttpServer::HttpServer(const ApiConfig& config)
    : config_(config),
    address_(config.getAddress()),
    port_(config.getPort()),
    thread_count_(config.getThreadCount()) {
}

HttpServer::~HttpServer() {
    if (running_) {
        stop();
    }
    wait_for_threads();
}

bool HttpServer::start() {
    try {
        if (running_) {
            ConsoleHelper::Error("[HTTP] 서버가 이미 실행 중입니다.");
            return false;
        }

        ConsoleHelper::Out("[HTTP] 서버 시작 중...");
        ConsoleHelper::Out(config_.toString());

        // IO Context 초기화
        ioc_ = std::make_unique<net::io_context>(thread_count_);

        // TCP 엔드포인트 및 어셉터 초기화
        tcp::endpoint endpoint(net::ip::make_address(address_), port_);
        acceptor_ = std::make_unique<tcp::acceptor>(*ioc_, endpoint);

        // 연결 수락 시작
        do_accept();

        // 워커 스레드 시작
        threads_.clear();
        threads_.reserve(thread_count_);

        for (int i = 0; i < thread_count_; ++i) {
            threads_.emplace_back([this, i]() {
                try {
                    ConsoleHelper::Out("[HTTP] 워커 스레드 #" + std::to_string(i) + " 시작");
                    ioc_->run();
                    ConsoleHelper::Out("[HTTP] 워커 스레드 #" + std::to_string(i) + " 종료");
                }
                catch (const std::exception& e) {
                    ConsoleHelper::Error("[HTTP] 워커 스레드 #" + std::to_string(i) + " 예외: " + e.what());
                }
                });
        }

        running_ = true;
        stopping_ = false;

        ConsoleHelper::OutWithColor(
            "[HTTP] API 서버 시작 완료 (주소: " + getAddress() + ")",
            ConsoleColor::Green);

        return true;
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error("[HTTP] 서버 시작 실패: " + std::string(e.what()));
        return false;
    }
}

void HttpServer::stop() {
    if (stopping_ || !running_) {
        return;
    }

    ConsoleHelper::OutWithColor("[HTTP] 서버 종료 중...", ConsoleColor::Yellow);
    stopping_ = true;

    try {
        if (acceptor_ && acceptor_->is_open()) {
            acceptor_->close();
        }

        if (ioc_) {
            ioc_->stop();
        }

        running_ = false;
        stopping_ = false;

        ConsoleHelper::OutWithColor("[HTTP] 서버가 정상적으로 종료되었습니다.", ConsoleColor::Green);
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error("[HTTP] 서버 종료 중 오류: " + std::string(e.what()));
        running_ = false;
        stopping_ = false;
    }
}

std::string HttpServer::getAddress() const {
    return "http://" + address_ + ":" + std::to_string(port_);
}

void HttpServer::do_accept() {
    if (stopping_) {
        return;
    }

    acceptor_->async_accept(
        [this](beast::error_code ec, tcp::socket socket) {
            if (stopping_) {
                return;
            }

            if (!ec) {
                ConsoleHelper::Out("[HTTP] 새 클라이언트 연결");
                std::make_shared<HttpSession>(std::move(socket))->start();
            }
            else if (ec != net::error::operation_aborted) {
                ConsoleHelper::Error("[HTTP] Accept 오류: " + ec.message());
            }

            if (!stopping_) {
                do_accept();
            }
        });
}

void HttpServer::wait_for_threads() {
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    threads_.clear();
}
