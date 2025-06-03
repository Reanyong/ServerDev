// WebSocketServer.cpp
#include "WebSocketServer.h"
#include "Session.h"
#include "../config/ServerConfig.h"
#include "../utils/ConsoleHelper.h"
#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <system_error>
#include <chrono>
#include <thread>

WebSocketServer::WebSocketServer(const ServerConfig& config)
    : address_(config.getServerAddress()),
    port_(config.getServerPort()),
    thread_count_(config.getThreadCount()) {
    // 기본 설정으로 초기화
    if (thread_count_ <= 0) {
        thread_count_ = std::thread::hardware_concurrency();
    }
}

WebSocketServer::~WebSocketServer() {
    // 아직 종료되지 않았다면 종료
    if (running_ && !stopping_) {
        stop();
    }

    // 모든 스레드가 종료될 때까지 대기
    wait_for_threads();
}

bool WebSocketServer::start() {
    try {
        // 이미 실행 중이면 오류
        if (running_) {
            ConsoleHelper::Error("[Server] 서버가 이미 실행 중입니다.");
            return false;
        }

        // IO Context 초기화
        ioc_ = std::make_unique<net::io_context>(thread_count_);

        // 스트랜드 초기화
        strands_.clear();
        for (int i = 0; i < thread_count_; ++i) {
            strands_.emplace_back(net::make_strand(*ioc_));
        }

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
                    ConsoleHelper::ThreadOut("[Thread] 워커 스레드 #" + std::to_string(i) + " 시작");

                    // IO Context 실행 - 작업이 없으면 블로킹됨
                    ioc_->run();

                    ConsoleHelper::ThreadOut("[Thread] 워커 스레드 #" + std::to_string(i) + " 종료");
                }
                catch (const std::exception& e) {
                    ConsoleHelper::Error("[Thread] 워커 스레드 #" + std::to_string(i) + " 예외: " + e.what());
                }
                });
        }

        running_ = true;
        stopping_ = false;

        ConsoleHelper::OutWithColor(
            "[Server] WebSocket 서버 시작 (주소: " + address_ + ":" + std::to_string(port_) +
            ", 스레드: " + std::to_string(thread_count_) + ")",
            ConsoleColor::Green);

        return true;
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error("[Server] 서버 시작 실패: " + std::string(e.what()));
        return false;
    }
}

void WebSocketServer::stop() {
    // 이미 중지 중이거나 실행 중이 아니면 무시
    if (stopping_ || !running_) {
        return;
    }

    ConsoleHelper::OutWithColor("[Server] 서버 종료 중...", ConsoleColor::Yellow);

    // 중지 중 상태로 설정
    stopping_ = true;

    try {
        // 어셉터 닫기
        if (acceptor_ && acceptor_->is_open()) {
            beast::error_code ec;
            acceptor_->close(ec);
            if (ec && ec != beast::errc::operation_canceled) {
                ConsoleHelper::Error("[Server] Acceptor 닫기 실패: " + ec.message());
            }
        }

        // 모든 IO 작업 취소
        if (ioc_) {
            ioc_->stop();
        }

        // 스레드가 종료될 때까지 대기 (타임아웃 추가)
        wait_for_threads();

        // 완전히 종료됨으로 설정
        running_ = false;
        stopping_ = false;
        connection_count_ = 0;

        ConsoleHelper::OutWithColor("[Server] 서버가 정상적으로 종료되었습니다.", ConsoleColor::Green);
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error("[Server] 서버 종료 중 오류: " + std::string(e.what()));
        // 오류가 발생해도 상태는 리셋
        running_ = false;
        stopping_ = false;
    }
}

std::string WebSocketServer::getServerAddress() const {
    if (port_ == 80) {
        return "ws://" + address_;
    }
    else {
        return "ws://" + address_ + ":" + std::to_string(port_);
    }
}

void WebSocketServer::registerSession(std::shared_ptr<Session> session) {
    if (session) {
        connection_count_++;
        ConsoleHelper::ThreadOut("[Server] 세션 등록 완료, 연결 수: " + std::to_string(connection_count_));
    }
}

void WebSocketServer::unregisterSession(std::shared_ptr<Session> session) {
    if (session && connection_count_ > 0) {
        connection_count_--;
        ConsoleHelper::ThreadOut("[Server] 세션 해제 완료, 연결 수: " + std::to_string(connection_count_));
        
        // 연결 해제 콜백 호출
        if (onClientDisconnected_) {
            onClientDisconnected_(session);
        }
    }
}

void WebSocketServer::do_accept() {
    // 서버가 종료 중이면 더 이상 연결을 수락하지 않음
    if (stopping_) {
        return;
    }

    // 다음 연결은 다른 스트랜드에서 처리하도록 설정
    int strand_idx = next_strand_index_++;
    if (next_strand_index_ >= static_cast<int>(strands_.size())) {
        next_strand_index_ = 0;  // 라운드 로빈 방식
    }

    // 비동기 연결 수락
    acceptor_->async_accept(
        net::bind_executor(
            strands_[strand_idx],
            [this, strand_idx](beast::error_code ec, tcp::socket socket) {
                if (stopping_) {
                    return;
                }

                if (!ec) {
                    ConsoleHelper::ThreadOut("[Server] 새 클라이언트 연결 수락됨");

                    try {
                        // 다른 스트랜드에서 세션 시작
                        int session_strand_idx = next_strand_index_++;
                        if (next_strand_index_ >= static_cast<int>(strands_.size())) {
                            next_strand_index_ = 0;
                        }

                        // 세션 생성 - ChatRoom을 Session에 전달
                        auto session = std::make_shared<Session>(std::move(socket), chat_room_);

                        // 다른 스트랜드에서 세션 시작 작업 포스팅 (connection_count는 성공 시에만 증가)
                        net::post(
                            strands_[session_strand_idx],
                            [this, session, session_strand_idx]() {
                                try {
                                    ConsoleHelper::ThreadOut("[Session] 세션 시작 시도");

                                    // 명시적으로 세션의 시작 함수를 호출 (핸드셰이크 수행)
                                    session->start();
                                    
                                    // 연결 성공 시 카운트 증가 (Session에서 처리하도록 변경)
                                    // connection_count_++;
                                    
                                    // WebSocket 핸드셰이크 완료 후 콜백 호출 (로그용)
                                    if (onClientConnected_) {
                                        onClientConnected_(session);
                                    }
                                    
                                    ConsoleHelper::ThreadOut("[Session] 세션 시작 완료");
                                }
                                catch (const std::exception& e) {
                                    ConsoleHelper::Error("[Error] 세션 시작 중 예외: " + std::string(e.what()));
                                    // 예외 발생 시 연결 카운트 증가하지 않음
                                }
                            });
                    }
                    catch (const std::exception& e) {
                        ConsoleHelper::Error("[Error] 클라이언트 연결 처리 중 예외: " + std::string(e.what()));
                    }
                }
                else {
                    // 일반적인 오류가 아닌 경우에만 로그 출력
                    if (ec != beast::errc::operation_canceled) {
                        ConsoleHelper::Error("[Error] Accept: " + ec.message());
                    }
                }

                // 종료 중이 아니면 다음 연결 수락 계속
                if (!stopping_) {
                    // 다음 클라이언트 연결 대기 - 다른 스트랜드에서 처리
                    int next_accept_strand_idx = next_strand_index_++;
                    if (next_strand_index_ >= static_cast<int>(strands_.size())) {
                        next_strand_index_ = 0;
                    }

                    // 다음 accept 작업을 다른 스트랜드에 포스팅
                    net::post(
                        strands_[next_accept_strand_idx],
                        [this]() {
                            do_accept();
                        });
                }
            }));
}

void WebSocketServer::wait_for_threads() {
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            try {
                thread.join();
            }
            catch (const std::exception& e) {
                ConsoleHelper::Error("[Thread] 스레드 종료 대기 중 예외: " + std::string(e.what()));
            }
        }
    }

    // 스레드 벡터 비우기
    threads_.clear();
}
