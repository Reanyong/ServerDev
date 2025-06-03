// ChatServerImpl.cpp
#include "ChatServerImpl.h"
#include "config/ServerConfig.h"
#include "net/WebSocketServer.h"
#include "net/Session.h"
#include "chat/ChatRoom.h"
#include "chat/ChatMessage.h"
#include "db/DatabaseManager.h"
#include "db/ChatRepository.h"
#include "utils/ConsoleHelper.h"
#include <memory>
#include <chrono>
#include <thread>
#include <map>
#include <sstream>
#include <atomic>

ChatServerImpl::ChatServerImpl(const ServerConfig& config)
    : config_(config), initialized_(false) {
    // 설정 정보 저장
}

ChatServerImpl::~ChatServerImpl() {
    // 실행 중인 서버 종료
    if (IsRunning()) {
        Stop();
    }
}

bool ChatServerImpl::Initialize() {
    try {
        // 이미 초기화되었다면 중복 초기화 방지
        if (initialized_) {
            ConsoleHelper::Out("[ChatServer] 이미 초기화되었습니다.");
            return true;
        }

        ConsoleHelper::Out("[ChatServer] 초기화 중...");

        // 데이터베이스 초기화
        bool db_success = initializeDatabase();
        if (!db_success) {
            ConsoleHelper::OutWithColor("[ChatServer] 데이터베이스 없이 서버 시작 (DB 기능 비활성화)", ConsoleColor::Yellow);
        }

        // ChatRepository 인스턴스 생성 (DB가 있을 때만)
        if (db_manager_) {
            chat_repository_ = std::make_unique<ChatRepository>(*db_manager_);
        } else {
            ConsoleHelper::Out("[ChatServer] ChatRepository 생성 생략 (DB 없음)");
        }

        // ChatRoom 인스턴스 생성
        chat_room_ = std::make_shared<ChatRoom>();
        if (!chat_room_->initialize()) {
            ConsoleHelper::Error("[ChatServer] 채팅방 초기화 실패");
            return false;
        }

        // WebSocketServer 인스턴스 생성
        ws_server_ = std::make_unique<WebSocketServer>(config_);

        // WebSocketServer에 ChatRoom 설정
        ws_server_->setChatRoom(chat_room_);

        // 클라이언트 연결/해제 콜백 설정
        ws_server_->setOnClientConnected([this](std::shared_ptr<Session> session) {
            onClientConnected(session);
            });

        ws_server_->setOnClientDisconnected([this](std::shared_ptr<Session> session) {
            onClientDisconnected(session);
            });

        initialized_ = true;
        log("채팅 서버 초기화 성공");
        return true;

    }
    catch (const std::exception& e) {
        ConsoleHelper::Error("[ChatServer] 초기화 실패: " + std::string(e.what()));
        return false;
    }
}

bool ChatServerImpl::Start() {
    if (!initialized_) {
        ConsoleHelper::Error("[ChatServer] 서버가 초기화되지 않았습니다. Initialize()를 먼저 호출하세요.");
        return false;
    }

    if (IsRunning()) {
        ConsoleHelper::Error("[ChatServer] 서버가 이미 실행 중입니다.");
        return false;
    }

    try {
        log("채팅 서버 시작 중...");

        // WebSocketServer 시작
        if (!ws_server_->start()) {
            ConsoleHelper::Error("[ChatServer] WebSocket 서버 시작 실패");
            return false;
        }

        log("채팅 서버가 시작되었습니다. (주소: " + GetServerAddress() + ")");
        return true;

    }
    catch (const std::exception& e) {
        ConsoleHelper::Error("[ChatServer] 시작 실패: " + std::string(e.what()));
        return false;
    }
}

void ChatServerImpl::Stop() {
    if (!IsRunning()) {
        return;
    }

    try {
        log("채팅 서버 종료 시작...");

        // 1. 먼저 새로운 연결을 차단하고 WebSocketServer 종료
        if (ws_server_) {
            try {
                log("WebSocketServer 정지 중...");
                ws_server_->stop();
                log("WebSocketServer 정지 완료");
            }
            catch (const std::runtime_error& e) {
                log("WebSocketServer 정지 중 runtime_error: " + std::string(e.what()));
            }
            catch (const std::exception& e) {
                log("WebSocketServer 정지 중 예외: " + std::string(e.what()));
            }
            catch (...) {
                log("WebSocketServer 정지 중 알 수 없는 예외");
            }
        }

        // 2. 채팅방의 모든 세션 정리
        if (chat_room_) {
            try {
                log("채팅방 정리 시작...");
                // 채팅방에 종료 메시지 브로드캐스트
                auto shutdown_msg = ChatMessage::createSystemMessage("서버가 종료됩니다. 연결이 곧 끊어집니다.");
                chat_room_->broadcast(shutdown_msg.toJson());
                
                // 잠시 기다려서 메시지가 전송되도록 함
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                
                // 모든 세션 종료
                chat_room_->closeAllSessions();
                log("채팅방 정리 완료");
            }
            catch (const std::runtime_error& e) {
                log("채팅방 정리 중 runtime_error: " + std::string(e.what()));
            }
            catch (const std::exception& e) {
                log("채팅방 정리 중 예외: " + std::string(e.what()));
            }
            catch (...) {
                log("채팅방 정리 중 알 수 없는 예외");
            }
        }

        // 3. ChatRepository 정리
        if (chat_repository_) {
            try {
                log("채팅 저장소 정리 시작...");
                chat_repository_.reset();
                log("채팅 저장소 정리 완료");
            }
            catch (const std::runtime_error& e) {
                log("채팅 저장소 정리 중 runtime_error (무시됨): " + std::string(e.what()));
            }
            catch (const std::exception& e) {
                log("채팅 저장소 정리 중 예외 (무시됨): " + std::string(e.what()));
            }
            catch (...) {
                log("채팅 저장소 정리 중 알 수 없는 예외 (무시됨)");
            }
        }

        // 4. 데이터베이스 매니저 정리 (가장 마지막)
        if (db_manager_) {
            try {
                log("데이터베이스 매니저 정리 시작...");
                // Release 모드에서도 안전한 정리를 위해 단계적 접근
                db_manager_.reset();
                log("데이터베이스 매니저 정리 완료");
            }
            catch (const std::runtime_error& e) {
                log("데이터베이스 매니저 정리 중 runtime_error (무시됨): " + std::string(e.what()));
            }
            catch (const std::exception& e) {
                log("데이터베이스 매니저 정리 중 예외 (무시됨): " + std::string(e.what()));
            }
            catch (...) {
                log("데이터베이스 매니저 정리 중 알 수 없는 예외 (무시됨)");
            }
        }

        log("채팅 서버 종료 완료");

    }
    catch (const std::runtime_error& e) {
        ConsoleHelper::Error("[ChatServer] 종료 중 runtime_error: " + std::string(e.what()));
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error("[ChatServer] 종료 중 예외: " + std::string(e.what()));
    }
    catch (...) {
        ConsoleHelper::Error("[ChatServer] 종료 중 알 수 없는 예외");
    }
}

bool ChatServerImpl::IsRunning() const {
    return ws_server_ && ws_server_->isRunning();
}

size_t ChatServerImpl::GetConnectionCount() const {
    return ws_server_ ? ws_server_->getConnectionCount() : 0;
}

std::string ChatServerImpl::GetServerAddress() const {
    return ws_server_ ? ws_server_->getServerAddress() : "";
}

uint16_t ChatServerImpl::GetServerPort() const {
    return ws_server_ ? ws_server_->getServerPort() : 0;
}

void ChatServerImpl::SetLogCallback(LogCallback callback) {
    log_callback_ = callback;
}

std::string ChatServerImpl::GetServerStats() const {
    std::stringstream ss;

    // 기본 서버 정보
    ss << "=== 서버 상태 정보 ===\n";
    ss << "실행 중: " << (IsRunning() ? "예" : "아니오") << "\n";
    ss << "연결 수: " << GetConnectionCount() << "\n";

    // 채팅방 정보
    if (chat_room_) {
        ss << "채팅방 ID: " << chat_room_->getChatId() << "\n";
        ss << "채팅방 참가자 수: " << chat_room_->getSessionCount() << "\n";
    }

    // DB 정보
    if (db_manager_ && chat_repository_) {
        ss << "활성 사용자 수: " << chat_repository_->getActiveUserCount() << "\n";

        // 상위 채팅 사용자
        auto topChatters = chat_repository_->getTopChatters(5);
        if (!topChatters.empty()) {
            ss << "\n상위 5명 사용자:\n";
            for (const auto& [username, count] : topChatters) {
                ss << "- " << username << ": " << count << "개 메시지\n";
            }
        }
    }

    return ss.str();
}

void ChatServerImpl::onClientConnected(std::shared_ptr<Session> session) {
    if (!session) return;

    try {
        // 단순히 로그만 남김 (ChatRoom 입장은 Session에서 처리)
        log("클라이언트 연결 완료: " + session->getNickname());

    }
    catch (const std::exception& e) {
        ConsoleHelper::Error("[ChatServer] 클라이언트 연결 처리 중 오류: " + std::string(e.what()));
    }
}

void ChatServerImpl::onClientDisconnected(std::shared_ptr<Session> session) {
    if (!session) return;

    try {
        std::string nickname = session->getNickname();

        // 퇴장 메시지 브로드캐스트 (Session에서 이미 처리되므로 여기서는 제거)
        // auto leave_msg = ChatMessage::createLeaveMessage(nickname);
        // chat_room_->broadcast(leave_msg.toJson(), session);

        // 채팅방에서 제거 (Session에서 이미 처리되므로 여기서는 제거)
        // chat_room_->leave(session);

        // WebSocketServer의 연결 카운트 감소
        if (ws_server_) {
            ws_server_->decrementConnectionCount();
        }

        log("클라이언트 연결 종료: " + nickname);

    }
    catch (const std::exception& e) {
        ConsoleHelper::Error("[ChatServer] 클라이언트 연결 종료 처리 중 오류: " + std::string(e.what()));
    }
}

void ChatServerImpl::log(const std::string& message) {
    // 콘솔에 로그 출력
    ConsoleHelper::Out("[ChatServer] " + message);

    // 콜백이 설정되어 있으면 호출
    if (log_callback_) {
        log_callback_(message);
    }
}

bool ChatServerImpl::initializeDatabase() {
    try {
        // DB 연결 문자열 생성
        std::string conn_str = config_.getDbConnectionString();

        // DatabaseManager 초기화 시도
        try {
            DatabaseManager::initialize(conn_str);
            db_manager_ = std::shared_ptr<DatabaseManager>(&DatabaseManager::getInstance(),
                [](DatabaseManager*) {}); // 싱글톤이므로 삭제하지 않음

            // 연결 테스트
            if (!db_manager_->testConnection()) {
                ConsoleHelper::Error("[DB] 데이터베이스 연결 테스트 실패 - DB 기능 비활성화");
                db_manager_.reset();
                return false; // DB 없이도 서버 동작하도록 false 반환하지만 계속 진행
            }

            ConsoleHelper::Out("[DB] 데이터베이스 연결 성공");
            return true;
        }
        catch (const std::exception& e) {
            ConsoleHelper::Error("[DB] DatabaseManager 초기화 실패: " + std::string(e.what()) + " - DB 기능 비활성화");
            db_manager_.reset();
            return false; // DB 없이도 서버 동작
        }
        catch (...) {
            ConsoleHelper::Error("[DB] DatabaseManager 초기화 중 알 수 없는 예외 - DB 기능 비활성화");
            db_manager_.reset();
            return false; // DB 없이도 서버 동작
        }

    }
    catch (const std::exception& e) {
        ConsoleHelper::Error("[DB] 데이터베이스 초기화 실패: " + std::string(e.what()) + " - DB 기능 비활성화");
        return false; // DB 없이도 서버 동작
    }
    catch (...) {
        ConsoleHelper::Error("[DB] 데이터베이스 초기화 중 알 수 없는 예외 - DB 기능 비활성화");
        return false; // DB 없이도 서버 동작
    }
}

// 팩토리 함수 구현
std::unique_ptr<IChatServer> CreateChatServer(const ServerConfig& config) {
    return std::make_unique<ChatServerImpl>(config);
}
