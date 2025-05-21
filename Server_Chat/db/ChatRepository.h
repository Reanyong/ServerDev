// ChatRepository.h
#pragma once

#include <string>
#include <vector>
#include <optional>
#include <tuple>
#include <pqxx/pqxx>
#include "..\chat\ChatMessage.h"

class DatabaseManager;

// 채팅 관련 저장소 클래스
class ChatRepository {
public:
    // 생성자/소멸자
    ChatRepository(DatabaseManager& dbManager);
    ~ChatRepository() = default;

    // 채팅방 관련 기능
    int createChatRoom(const std::string& chatName);
    std::optional<int> findChatRoomByName(const std::string& chatName);
    bool updateChatRoom(int chatId, const std::string& newName);
    bool deleteChatRoom(int chatId);

    // 유저 관련 기능
    std::string registerUser(const std::string& username, const std::string& email = "");
    std::optional<std::string> findUserByUsername(const std::string& username);
    bool updateUserUsername(const std::string& userId, const std::string& newUsername);

    // 세션 관련 기능
    std::string createSession(const std::string& userId, const std::string& status = "active");
    bool updateSessionStatus(const std::string& userId, const std::string& status);
    bool endAllActiveSessions(const std::string& userId);

    // 메시지 관련 기능
    bool saveMessage(const ChatMessage& message, int chatId, const std::string& userId = "");
    std::vector<ChatMessage> getRecentMessages(int chatId, int limit = 50);
    std::vector<ChatMessage> getUserMessages(const std::string& userId, int limit = 50);

    // 통계 관련 기능
    int getActiveUserCount();
    std::vector<std::tuple<std::string, int>> getTopChatters(int limit = 10);

    // 유틸리티 기능
    bool isTableExists(const std::string& tableName);
    bool createTablesIfNotExist();

private:
    DatabaseManager& db_manager_;

    // 내부 헬퍼 함수
    void ensureTablesExist();
    ChatMessage resultRowToChatMessage(const pqxx::row& row);
    std::string generateUUID(); // UUID 생성 함수 추가
};
