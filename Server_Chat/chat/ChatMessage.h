// ChatMessage.h
#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include <chrono>
#include <optional>

enum class MessageType {
    JOIN,       // 사용자 입장
    LEAVE,      // 사용자 퇴장
    CHAT,       // 일반 채팅
    SYSTEM,     // 시스템 메시지
    WHISPER,    // 귓속말 (새로 추가)
    STATUS      // 상태 업데이트 (새로 추가)
};

class ChatMessage {
public:
    // 기본 생성자
    ChatMessage() = default;

    // 완전한 생성자
    ChatMessage(MessageType type, const std::string& nickname, const std::string& content,
        const std::string& timestamp = "", const std::string& targetNickname = "");

    // 게터 메소드들
    MessageType getType() const { return type_; }
    const std::string& getNickname() const { return nickname_; }
    const std::string& getContent() const { return content_; }
    const std::string& getTimestamp() const { return timestamp_; }
    const std::string& getTargetNickname() const { return target_nickname_; }

    // JSON 직렬화/역직렬화
    std::string toJson() const;
    static ChatMessage fromJson(const std::string& jsonString);

    // 메시지 생성 헬퍼 함수들
    static ChatMessage createJoinMessage(const std::string& nickname);
    static ChatMessage createLeaveMessage(const std::string& nickname);
    static ChatMessage createChatMessage(const std::string& nickname, const std::string& content);
    static ChatMessage createSystemMessage(const std::string& content);
    static ChatMessage createWhisperMessage(const std::string& from, const std::string& to, const std::string& content);
    static ChatMessage createStatusMessage(const std::string& nickname, const std::string& status);

    // DB 저장 기능
    bool saveToDatabase(int chat_id = 1, const std::string& user_id = "") const;

private:
    MessageType type_ = MessageType::CHAT;
    std::string nickname_;
    std::string content_;
    std::string timestamp_;
    std::string target_nickname_;  // 귓속말 대상 (whisper 타입일 때만 사용)

    // 현재 타임스탬프 생성 함수
    static std::string getCurrentTimestamp();
};
