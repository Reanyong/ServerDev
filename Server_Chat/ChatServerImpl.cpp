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
        if (!initializeDatabase()) {
            ConsoleHelper::Error("[ChatServer] 데이터베이스 초기화 실패");
            return false;
        }

        // ChatRepository 인스턴스 생성
        chat_repository_ = std::make_unique<ChatRepository>(*db_manager_);

        // ChatRoom 인스턴스 생성
        chat_room_ = std::make_unique<ChatRoom>();
        if (!chat_room_->initialize()) {
            ConsoleHelper::Error("[ChatServer] 채팅방 초기화 실패");
            return false;
        }

        // WebSocketServer 인스턴스 생성
        ws_server_ = std::make_unique<WebSocketServer>(config_);

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
        log("채팅 서버 종료 중...");

        // WebSocketServer 종료
        ws_server_->stop();

        log("채팅 서버가 종료되었습니다.");

    }
    catch (const std::exception& e) {
        ConsoleHelper::Error("[ChatServer] 종료 중 오류: " + std::string(e.what()));
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
        // 고유 ID 할당
        int userId = chat_room_->generateUserId();
        std::string nickname = "User" + std::to_string(userId);
        session->setNickname(nickname);

        // 채팅방에 참가
        chat_room_->join(session);

        // 입장 메시지 브로드캐스트
        auto join_msg = ChatMessage::createJoinMessage(nickname);
        chat_room_->broadcast(join_msg.toJson());

        // Welcome 메세지 전송
        auto welcome_msg = ChatMessage::createSystemMessage(
            "환영합니다! 현재 " + std::to_string(chat_room_->getSessionCount()) + "명이 접속 중입니다.\n"
            "명령어 안내: /nick [새닉네임] - 닉네임 변경, /help - 도움말");
        session->send(welcome_msg.toJson());

        log("클라이언트 연결: " + nickname);

    }
    catch (const std::exception& e) {
        ConsoleHelper::Error("[ChatServer] 클라이언트 연결 처리 중 오류: " + std::string(e.what()));
    }
}

void ChatServerImpl::onClientDisconnected(std::shared_ptr<Session> session) {
    if (!session) return;

    try {
        std::string nickname = session->getNickname();

        // 퇴장 메시지 브로드캐스트
        auto leave_msg = ChatMessage::createLeaveMessage(nickname);
        chat_room_->broadcast(leave_msg.toJson(), session);

        // 채팅방에서 제거
        chat_room_->leave(session);

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

        // DatabaseManager 초기화
        DatabaseManager::initialize(conn_str);
        db_manager_ = std::shared_ptr<DatabaseManager>(&DatabaseManager::getInstance(),
            [](DatabaseManager*) {}); // 싱글톤이므로 삭제하지 않음

        // 연결 테스트
        if (!db_manager_->testConnection()) {
            ConsoleHelper::Error("[DB] 데이터베이스 연결 테스트 실패");
            return false;
        }

        ConsoleHelper::Out("[DB] 데이터베이스 연결 성공");
        return true;

    }
    catch (const std::exception& e) {
        ConsoleHelper::Error("[DB] 데이터베이스 초기화 실패: " + std::string(e.what()));
        return false;
    }
}

// 팩토리 함수 구현
std::unique_ptr<IChatServer> CreateChatServer(const ServerConfig& config) {
    return std::make_unique<ChatServerImpl>(config);
}
