// ChatRepository.cpp
#include "ChatRepository.h"
#include "DatabaseManager.h"
#include "../utils/ConsoleHelper.h"
#include "../utils/StringUtils.h"
#include <pqxx/pqxx>
#include <sstream>
#include <iomanip>
#include <Windows.h>
#include <objbase.h> // Windows UUID 지원

// 생성자
ChatRepository::ChatRepository(DatabaseManager& dbManager)
    : db_manager_(dbManager) {
    // 필요한 테이블 존재 확인 및 생성
    //ensureTablesExist();
}

// 채팅방 생성
int ChatRepository::createChatRoom(const std::string& chatName) {
    try {
        auto result = db_manager_.executeQuery(
            "INSERT INTO chats (chat_name, created_at) VALUES ('" +
            chatName + "', NOW()) RETURNING id");

        if (!result.empty()) {
            return result[0][0].as<int>();
        }
        return -1;
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error(StringUtils::Utf8ToWString(std::string("채팅방 생성 실패: ") + e.what()));
        return -1;
    }
}

// 채팅방 이름으로 채팅방 ID 찾기
std::optional<int> ChatRepository::findChatRoomByName(const std::string& chatName) {
    try {
        auto result = db_manager_.executeQuery(
            "SELECT id FROM chats WHERE chat_name = '" + chatName + "' LIMIT 1");

        if (!result.empty()) {
            return result[0][0].as<int>();
        }
        return std::nullopt;
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error(StringUtils::Utf8ToWString(std::string("채팅방 조회 실패: ") + e.what()));
        return std::nullopt;
    }
}

// 채팅방 이름 업데이트
bool ChatRepository::updateChatRoom(int chatId, const std::string& newName) {
    try {
        auto result = db_manager_.executeQuery(
            "UPDATE chats SET chat_name = '" + newName +
            "' WHERE id = " + std::to_string(chatId));

        return true;
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error(StringUtils::Utf8ToWString(std::string("채팅방 업데이트 실패: ") + e.what()));
        return false;
    }
}

// 채팅방 삭제
bool ChatRepository::deleteChatRoom(int chatId) {
    try {
        // 관련 메시지 먼저 삭제
        db_manager_.executeQuery(
            "DELETE FROM messages WHERE chat_id = " + std::to_string(chatId));

        // 채팅방 삭제
        db_manager_.executeQuery(
            "DELETE FROM chats WHERE id = " + std::to_string(chatId));

        return true;
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error(StringUtils::Utf8ToWString(std::string("채팅방 삭제 실패: ") + e.what()));
        return false;
    }
}

// 사용자 등록
std::string ChatRepository::registerUser(const std::string& username, const std::string& email) {
    try {
        // UUID 생성
        std::string userId = generateUUID();

        std::string query = "INSERT INTO users (id, username, email, created_at) VALUES ('" +
            userId + "', '" + username + "', '" +
            email + "', NOW())";

        db_manager_.executeQuery(query);
        return userId;
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error(StringUtils::Utf8ToWString(std::string("사용자 등록 실패: ") + e.what()));
        return "";
    }
}

// 사용자명으로 사용자 ID 찾기
std::optional<std::string> ChatRepository::findUserByUsername(const std::string& username) {
    try {
        auto result = db_manager_.executeQuery(
            "SELECT id FROM users WHERE username = '" + username + "' LIMIT 1");

        if (!result.empty()) {
            return result[0][0].as<std::string>();
        }
        return std::nullopt;
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error(StringUtils::Utf8ToWString(std::string("사용자 조회 실패: ") + e.what()));
        return std::nullopt;
    }
}

// 사용자 이름 업데이트
bool ChatRepository::updateUserUsername(const std::string& userId, const std::string& newUsername) {
    try {
        db_manager_.executeQuery(
            "UPDATE users SET username = '" + newUsername +
            "' WHERE id = '" + userId + "'");

        return true;
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error(StringUtils::Utf8ToWString(std::string("사용자 업데이트 실패: ") + e.what()));
        return false;
    }
}

// 세션 생성
std::string ChatRepository::createSession(const std::string& userId, const std::string& status) {
    try {
        // UUID 생성
        std::string sessionId = generateUUID();

        std::string query = "INSERT INTO sessions (session_id, user_id, status, created_at) VALUES ('" +
            sessionId + "', '" + userId + "', '" + status + "', NOW())";

        db_manager_.executeQuery(query);
        return sessionId;
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error(StringUtils::Utf8ToWString(std::string("세션 생성 실패: ") + e.what()));
        return "";
    }
}

// 세션 상태 업데이트
bool ChatRepository::updateSessionStatus(const std::string& userId, const std::string& status) {
    try {
        db_manager_.executeQuery(
            "UPDATE sessions SET status = '" + status +
            "' WHERE user_id = '" + userId + "' AND status = 'active'");

        return true;
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error(StringUtils::Utf8ToWString(std::string("세션 상태 업데이트 실패: ") + e.what()));
        return false;
    }
}

// 모든 활성 세션 종료
bool ChatRepository::endAllActiveSessions(const std::string& userId) {
    try {
        db_manager_.executeQuery(
            "UPDATE sessions SET status = 'ended' WHERE user_id = '" +
            userId + "' AND status = 'active'");

        return true;
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error(StringUtils::Utf8ToWString(std::string("세션 종료 실패: ") + e.what()));
        return false;
    }
}

// 메시지 저장
bool ChatRepository::saveMessage(const ChatMessage& message, int chatId, const std::string& userId) {
    try {
        std::string messageJson = message.toJson();
        std::string effectiveUserId = userId.empty() ? "NULL" : "'" + userId + "'";

        std::string query = "INSERT INTO messages (chat_id, user_id, message, created_at) VALUES (" +
            std::to_string(chatId) + ", " + effectiveUserId + ", '" +
            messageJson + "', NOW())";

        db_manager_.executeQuery(query);
        return true;
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error(StringUtils::Utf8ToWString(std::string("메시지 저장 실패: ") + e.what()));
        return false;
    }
}

// 최근 메시지 가져오기
std::vector<ChatMessage> ChatRepository::getRecentMessages(int chatId, int limit) {
    std::vector<ChatMessage> messages;
    try {
        std::string query = "SELECT m.id, m.message, m.created_at, u.username "
            "FROM messages m "
            "LEFT JOIN users u ON m.user_id = u.id "
            "WHERE m.chat_id = " + std::to_string(chatId) + " "
            "ORDER BY m.created_at DESC LIMIT " + std::to_string(limit);

        auto result = db_manager_.executeQuery(query);

        for (const auto& row : result) {
            messages.push_back(resultRowToChatMessage(row));
        }
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error(StringUtils::Utf8ToWString(std::string("메시지 조회 실패: ") + e.what()));
    }
    return messages;
}

// 사용자별 메시지 가져오기
std::vector<ChatMessage> ChatRepository::getUserMessages(const std::string& userId, int limit) {
    std::vector<ChatMessage> messages;
    try {
        std::string query = "SELECT m.id, m.message, m.created_at, u.username "
            "FROM messages m "
            "JOIN users u ON m.user_id = u.id "
            "WHERE m.user_id = '" + userId + "' "
            "ORDER BY m.created_at DESC LIMIT " + std::to_string(limit);

        auto result = db_manager_.executeQuery(query);

        for (const auto& row : result) {
            messages.push_back(resultRowToChatMessage(row));
        }
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error(StringUtils::Utf8ToWString(std::string("사용자 메시지 조회 실패: ") + e.what()));
    }
    return messages;
}

// 활성 사용자 수 조회
int ChatRepository::getActiveUserCount() {
    try {
        auto result = db_manager_.executeQuery(
            "SELECT COUNT(DISTINCT user_id) FROM sessions WHERE status = 'active'");

        if (!result.empty()) {
            return result[0][0].as<int>();
        }
        return 0;
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error(StringUtils::Utf8ToWString(std::string("활성 사용자 수 조회 실패: ") + e.what()));
        return 0;
    }
}

// 상위 채팅 사용자 조회
std::vector<std::tuple<std::string, int>> ChatRepository::getTopChatters(int limit) {
    std::vector<std::tuple<std::string, int>> result;
    try {
        std::string query = "SELECT u.username, COUNT(m.id) as message_count "
            "FROM users u "
            "JOIN messages m ON u.id = m.user_id "
            "GROUP BY u.username "
            "ORDER BY message_count DESC LIMIT " + std::to_string(limit);

        auto queryResult = db_manager_.executeQuery(query);

        for (const auto& row : queryResult) {
            std::string username = row[0].as<std::string>();
            int messageCount = row[1].as<int>();
            result.emplace_back(username, messageCount);
        }
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error(StringUtils::Utf8ToWString(std::string("상위 채팅 사용자 조회 실패: ") + e.what()));
    }
    return result;
}

// 테이블 존재 여부 확인
bool ChatRepository::isTableExists(const std::string& tableName) {
    try {
        std::string query = "SELECT EXISTS (SELECT FROM information_schema.tables "
            "WHERE table_schema = 'public' "
            "AND table_name = '" + tableName + "')";

        auto result = db_manager_.executeQuery(query);

        if (!result.empty()) {
            return result[0][0].as<bool>();
        }
        return false;
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error(StringUtils::Utf8ToWString(std::string("테이블 존재 확인 실패: ") + e.what()));
        return false;
    }
}

// 필요한 테이블 생성
bool ChatRepository::createTablesIfNotExist() {
    try {
        // Users 테이블
        if (!isTableExists("users")) {
            db_manager_.executeQuery(
                "CREATE TABLE users ("
                "id UUID PRIMARY KEY, "
                "username VARCHAR(50) UNIQUE, "
                "email VARCHAR(100), "
                "created_at TIMESTAMPTZ"
                ")");
        }

        // Chats 테이블
        if (!isTableExists("chats")) {
            db_manager_.executeQuery(
                "CREATE TABLE chats ("
                "id SERIAL PRIMARY KEY, "
                "chat_name VARCHAR(100), "
                "created_at TIMESTAMPTZ"
                ")");
        }

        // Sessions 테이블
        if (!isTableExists("sessions")) {
            db_manager_.executeQuery(
                "CREATE TABLE sessions ("
                "session_id UUID PRIMARY KEY, "
                "user_id UUID REFERENCES users(id), "
                "status VARCHAR(20), "
                "created_at TIMESTAMPTZ"
                ")");
        }

        // Messages 테이블
        if (!isTableExists("messages")) {
            db_manager_.executeQuery(
                "CREATE TABLE messages ("
                "id SERIAL PRIMARY KEY, "
                "chat_id INT REFERENCES chats(id), "
                "user_id UUID REFERENCES users(id), "
                "message TEXT, "
                "created_at TIMESTAMPTZ"
                ")");
        }

        return true;
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error(StringUtils::Utf8ToWString(std::string("테이블 생성 실패: ") + e.what()));
        return false;
    }
}

// 내부 헬퍼 함수들
void ChatRepository::ensureTablesExist() {
    createTablesIfNotExist();
}

// DB 결과 행을 ChatMessage 객체로 변환
ChatMessage ChatRepository::resultRowToChatMessage(const pqxx::row& row) {
    int id = row[0].as<int>();
    std::string jsonStr = row[1].as<std::string>();
    std::string timestamp = row[2].as<std::string>();
    std::string username = row[3].is_null() ? "" : row[3].as<std::string>();

    // JSON 문자열로부터 ChatMessage 객체 생성
    ChatMessage message;
    message.fromJson(jsonStr);

    return message;
}

// UUID 생성 헬퍼 함수 (Windows 버전)
std::string ChatRepository::generateUUID() {
    GUID guid;
    HRESULT hr = CoCreateGuid(&guid);

    if (SUCCEEDED(hr)) {
        char uuid_str[37]; // 36자 UUID + null 문자
        sprintf_s(uuid_str,
            "%08lx-%04hx-%04hx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
            guid.Data1, guid.Data2, guid.Data3,
            guid.Data4[0], guid.Data4[1], guid.Data4[2],
            guid.Data4[3], guid.Data4[4], guid.Data4[5],
            guid.Data4[6], guid.Data4[7]);

        return std::string(uuid_str);
    }

    // 실패 시 임시 UUID 생성 (실제 프로덕션 코드에서는 사용하지 말 것)
    std::stringstream ss;
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto value = now_ms.time_since_epoch().count();
    ss << std::hex << value << "-temp-uuid";

    return ss.str();
}
