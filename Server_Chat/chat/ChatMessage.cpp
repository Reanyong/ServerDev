// ChatMessage.cpp
#include "ChatMessage.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include "../db/DatabaseManager.h"
#include "../utils/ConsoleHelper.h"
#include "../utils/StringUtils.h"

ChatMessage::ChatMessage(MessageType type, const std::string& nickname, const std::string& content,
    const std::string& timestamp, const std::string& targetNickname)
    : type_(type), nickname_(nickname), content_(content),
    timestamp_(timestamp.empty() ? getCurrentTimestamp() : timestamp),
    target_nickname_(targetNickname) {
}

std::string ChatMessage::toJson() const {
    nlohmann::json j;
    j["type"] = static_cast<int>(type_);
    j["nickname"] = nickname_;
    j["content"] = content_;
    j["timestamp"] = timestamp_;

    // 귓속말의 경우 대상 닉네임도 포함
    if (type_ == MessageType::WHISPER && !target_nickname_.empty()) {
        j["target_nickname"] = target_nickname_;
    }

    return j.dump();
}

ChatMessage ChatMessage::fromJson(const std::string& jsonString) {
    try {
        nlohmann::json j = nlohmann::json::parse(jsonString);

        MessageType type = static_cast<MessageType>(j["type"].get<int>());
        std::string nickname = j["nickname"].get<std::string>();
        std::string content = j["content"].get<std::string>();
        std::string timestamp = j["timestamp"].get<std::string>();

        // 대상 닉네임이 있으면 읽어옴
        std::string targetNickname;
        if (j.contains("target_nickname")) {
            targetNickname = j["target_nickname"].get<std::string>();
        }

        return ChatMessage(type, nickname, content, timestamp, targetNickname);
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error("JSON 파싱 실패: " + std::string(e.what()));
        // 기본 오류 메시지 생성
        return createSystemMessage("메시지 처리 중 오류가 발생했습니다.");
    }
}

ChatMessage ChatMessage::createJoinMessage(const std::string& nickname) {
    return ChatMessage(MessageType::JOIN, nickname, nickname + "님이 입장하셨습니다.");
}

ChatMessage ChatMessage::createLeaveMessage(const std::string& nickname) {
    return ChatMessage(MessageType::LEAVE, nickname, nickname + "님이 퇴장하셨습니다.");
}

ChatMessage ChatMessage::createChatMessage(const std::string& nickname, const std::string& content) {
    return ChatMessage(MessageType::CHAT, nickname, content);
}

ChatMessage ChatMessage::createSystemMessage(const std::string& content) {
    return ChatMessage(MessageType::SYSTEM, "System", content);
}

ChatMessage ChatMessage::createWhisperMessage(const std::string& from, const std::string& to, const std::string& content) {
    std::string whisperContent = "[귓속말] " + content;
    return ChatMessage(MessageType::WHISPER, from, whisperContent, "", to);
}

ChatMessage ChatMessage::createStatusMessage(const std::string& nickname, const std::string& status) {
    return ChatMessage(MessageType::STATUS, nickname, status);
}

bool ChatMessage::saveToDatabase(int chat_id, const std::string& user_id) const {
    try {
        auto& db = DatabaseManager::getInstance();
        return db.executeTransaction([&](pqxx::work& txn) {
            if (user_id.empty()) {
                // user_id가 없는 경우 (시스템 메시지 등)
                txn.exec(
                    "INSERT INTO Messages (chat_id, user_id, message, created_at, message_type, source) "
                    "VALUES ("
                    + txn.quote(chat_id) + ", "
                    + "NULL, "
                    + txn.quote(content_) + ", "
                    + txn.quote(timestamp_) + "::timestamptz, "
                    + txn.quote(static_cast<int>(type_)) + ", "
                    + "'cpp')"
                );
            }
            else {
                // user_id가 있는 경우
                txn.exec(
                    "INSERT INTO Messages (chat_id, user_id, message, created_at, message_type, source) "
                    "VALUES ("
                    + txn.quote(chat_id) + ", "
                    + txn.quote(user_id) + "::uuid, "
                    + txn.quote(content_) + ", "
                    + txn.quote(timestamp_) + "::timestamptz, "
                    + txn.quote(static_cast<int>(type_)) + ", "
                    + "'cpp')"
                );
            }
            });
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error("메시지 저장 실패: " + std::string(e.what()));
        return false;
    }
}

std::string ChatMessage::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    tm timeinfo;

#ifdef _MSC_VER
    localtime_s(&timeinfo, &in_time_t);
#else
    localtime_r(&in_time_t, &timeinfo);
#endif

    ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}
