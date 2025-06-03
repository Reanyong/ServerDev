// ChatRoom.h
#pragma once

// 전방 선언
class Session;
class ChatMessage;

#include <set>
#include <mutex>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <functional>

class ChatRoom {
public:
    // 생성자/소멸자
    ChatRoom();
    ~ChatRoom() = default;

    // 데이터베이스 채팅방 초기화
    bool initialize();

    // 채팅방 세션 관리
    void join(std::shared_ptr<Session> session);
    void leave(std::shared_ptr<Session> session);

    // 메시지 브로드캐스트
    void broadcast(const std::string& message, std::shared_ptr<Session> sender = nullptr);

    // 귓속말 기능 (특정 사용자에게만 메시지 전송)
    bool whisper(const std::string& targetNickname, const std::string& message, std::shared_ptr<Session> sender);

    // 서버 종료 시 모든 세션 닫기
    void closeAllSessions();

    // 유틸리티 함수들
    int generateUserId();
    size_t getSessionCount() const;
    int getChatId() const;

    // 사용자 검색 (닉네임으로)
    std::shared_ptr<Session> findSessionByNickname(const std::string& nickname);

    // 채팅방 상태 정보 얻기
    std::string getStatusJson() const;

private:
    // 세션 관리
    std::set<std::shared_ptr<Session>> sessions_;

    // 닉네임으로 세션 조회용 맵 (빠른 검색)
    std::unordered_map<std::string, std::weak_ptr<Session>> nickname_map_;

    // 동기화 관련
    mutable std::mutex mutex_;
    std::atomic<int> next_user_id_{ 1 };

    // 채팅방 DB 정보
    int chat_id_{ 1 };
    bool initialized_{ false };

    // 세션 맵 업데이트
    void updateNicknameMap(const std::string& oldNickname, const std::string& newNickname, std::shared_ptr<Session> session);

    // 메시지 DB 저장 처리 (별도 스레드)
    void saveMessageToDatabase(const ChatMessage& message, const std::string& user_id);
};
