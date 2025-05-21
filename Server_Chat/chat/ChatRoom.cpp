// ChatRoom.cpp
#include "ChatRoom.h"
#include "../net/Session.h"
#include "ChatMessage.h"
#include "../db/DatabaseManager.h"
#include "../utils/ConsoleHelper.h"
#include <thread>
#include <future>
#include <algorithm>
#include <sstream>

ChatRoom::ChatRoom() : chat_id_(1), initialized_(false) {
    // 기본 생성자 - 초기화는 initialize() 메서드에서 수행
}

bool ChatRoom::initialize() {
    if (initialized_) return true;  // 이미 초기화되었으면 성공 반환

    try {
        // 채팅방이 이미 있는지 확인하고 없으면 생성
        auto& db = DatabaseManager::getInstance();

        auto result = db.executeQuery("SELECT id FROM Chats WHERE chat_name = '기본 채팅방' LIMIT 1");

        if (result.empty()) {
            // 트랜잭션으로 채팅방 생성
            db.executeTransaction([&](pqxx::work& txn) {
                pqxx::result insert_result = txn.exec(
                    "INSERT INTO Chats (chat_name) VALUES ('기본 채팅방') RETURNING id"
                );
                if (!insert_result.empty()) {
                    chat_id_ = insert_result[0][0].as<int>();
                }
                });

            ConsoleHelper::Out("[DB] 새 채팅방 생성 - ID: " + std::to_string(chat_id_));
        }
        else {
            // 기존 채팅방 ID 사용
            chat_id_ = result[0][0].as<int>();
            ConsoleHelper::Out("[DB] 기존 채팅방 사용 - ID: " + std::to_string(chat_id_));
        }

        initialized_ = true;
        return true;
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error("채팅방 초기화 오류: " + std::string(e.what()));
        // 초기화 실패시 기본값으로 진행
        chat_id_ = 1;
        return false;
    }
}

void ChatRoom::join(std::shared_ptr<Session> session) {
    if (!session) return;

    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.insert(session);

    // 닉네임 맵 업데이트
    nickname_map_[session->getNickname()] = session;

    ConsoleHelper::Out("[ChatRoom] 사용자 입장: " + session->getNickname() +
        " (현재 " + std::to_string(sessions_.size()) + "명)");
}

void ChatRoom::leave(std::shared_ptr<Session> session) {
    if (!session) return;

    std::lock_guard<std::mutex> lock(mutex_);

    // 닉네임 맵에서 제거
    auto it = nickname_map_.find(session->getNickname());
    if (it != nickname_map_.end()) {
        nickname_map_.erase(it);
    }

    // 세션 집합에서 제거
    sessions_.erase(session);

    ConsoleHelper::Out("[ChatRoom] 사용자 퇴장: " + session->getNickname() +
        " (현재 " + std::to_string(sessions_.size()) + "명)");
}

void ChatRoom::broadcast(const std::string& message, std::shared_ptr<Session> sender) {
    // 메시지를 DB에 저장
    try {
        ChatMessage chat_msg = ChatMessage::fromJson(message);

        // 별도 스레드에서 DB 저장 작업 실행
        /*std::thread([chat_msg, this, sender]() {*/
        std::thread([chat_msg, chat_id = this->chat_id_, sender]() {
            try {
                if (sender) {
                    std::string user_id = sender->getDbUserId();
                    if (!user_id.empty()) {
                        chat_msg.saveToDatabase(chat_id, user_id);
                        ConsoleHelper::Out("[DB] 메시지 저장 성공 (사용자: " + user_id + ")");
                    }
                    else {
                        chat_msg.saveToDatabase(chat_id);
                        ConsoleHelper::Out("[DB] 메시지 저장 성공 (시스템)");
                    }
                }
                else {
                    chat_msg.saveToDatabase(chat_id);
                    ConsoleHelper::Out("[DB] 메시지 저장 성공 (시스템)");
                }
            }
            catch (const std::exception& e) {
                ConsoleHelper::Error("[DB] 메시지 저장 중 예외: " + std::string(e.what()));
            }
            }).detach();
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error("[Error] 메시지 파싱 실패: " + std::string(e.what()));
    }

    // 메시지 브로드캐스트
    std::vector<std::weak_ptr<Session>> targets;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& session : sessions_) {
            if (session != sender) {
                targets.push_back(session);
            }
        }
    }

    // 잠금 없이 안전하게 메시지 전송
    for (auto& weak_session : targets) {
        if (auto session = weak_session.lock()) {
            try {
                if (session->is_open()) {
                    session->send(message);
                }
            }
            catch (const std::exception& e) {
                ConsoleHelper::Error("[Error] 메시지 전송 중 예외: " + std::string(e.what()));
            }
        }
    }
}

bool ChatRoom::whisper(const std::string& targetNickname, const std::string& message, std::shared_ptr<Session> sender) {
    if (!sender) return false;

    std::shared_ptr<Session> targetSession;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = nickname_map_.find(targetNickname);
        if (it != nickname_map_.end()) {
            targetSession = it->second.lock();
        }
    }

    if (!targetSession || !targetSession->is_open()) {
        return false;
    }

    try {
        // 귓속말 메시지 생성
        auto whisperMsg = ChatMessage::createWhisperMessage(
            sender->getNickname(), targetNickname, message);

        // 대상자에게 전송
        targetSession->send(whisperMsg.toJson());

        // 발신자에게도 동일 메시지 전송 (자신이 보낸 귓속말 확인용)
        sender->send(whisperMsg.toJson());

        // DB에 저장
        saveMessageToDatabase(whisperMsg, sender->getDbUserId());

        return true;
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error("[Error] 귓속말 전송 중 예외: " + std::string(e.what()));
        return false;
    }
}

int ChatRoom::generateUserId() {
    return next_user_id_++;
}

size_t ChatRoom::getSessionCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.size();
}

int ChatRoom::getChatId() const {
    return chat_id_;
}

std::shared_ptr<Session> ChatRoom::findSessionByNickname(const std::string& nickname) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nickname_map_.find(nickname);
    if (it != nickname_map_.end()) {
        return it->second.lock();
    }
    return nullptr;
}

std::string ChatRoom::getStatusJson() const {
    std::lock_guard<std::mutex> lock(mutex_);

    nlohmann::json status;
    status["room_id"] = chat_id_;
    status["user_count"] = sessions_.size();

    nlohmann::json users = nlohmann::json::array();
    for (const auto& session : sessions_) {
        if (session) {
            nlohmann::json user;
            user["nickname"] = session->getNickname();
            user["id"] = session->getUserId();
            users.push_back(user);
        }
    }
    status["users"] = users;

    return status.dump();
}

void ChatRoom::updateNicknameMap(const std::string& oldNickname, const std::string& newNickname, std::shared_ptr<Session> session) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 기존 닉네임 제거
    if (!oldNickname.empty()) {
        nickname_map_.erase(oldNickname);
    }

    // 새 닉네임 추가
    if (!newNickname.empty() && session) {
        nickname_map_[newNickname] = session;
    }
}

void ChatRoom::saveMessageToDatabase(const ChatMessage& message, const std::string& user_id) {
    try {
        // 비동기적으로 DB에 저장
        std::thread([message, user_id, this]() {
            try {
                message.saveToDatabase(this->chat_id_, user_id);
            }
            catch (const std::exception& e) {
                ConsoleHelper::Error("[DB] 메시지 저장 실패: " + std::string(e.what()));
            }
            }).detach();
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error("[Error] 메시지 저장 스레드 생성 실패: " + std::string(e.what()));
    }
}
