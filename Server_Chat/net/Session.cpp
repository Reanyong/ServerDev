// Session.cpp
#include "Session.h"
#include "../chat/ChatRoom.h"
#include "../chat/ChatMessage.h"
#include "../utils/ConsoleHelper.h"
#include "../utils/StringUtils.h"
#include "../db/DatabaseManager.h"
#include <chrono>
#include <iostream>

Session::Session(tcp::socket socket)
    : ws_(std::move(socket)) {
    // WebSocket 옵션 설정
    ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));

    // 기본 사용자 정보 설정
    user_id_ = 0;  // 실제 ID는 채팅방에서 할당
    nickname_ = "Guest";  // 기본 닉네임
}

Session::~Session() {
    try {
        // 이미 closed_ 플래그가 설정되어 있지 않다면 명시적으로 close 호출
        if (!closed_) {
            beast::error_code ec;
            ws_.close(websocket::close_code::normal, ec);
            closed_ = true;
        }
    }
    catch (...) {
        // 소멸자에서는 예외 무시
    }
}

void Session::start() {
    // 현재 스레드 ID 출력
    ConsoleHelper::ThreadOut("[Session] 웹소켓 핸드셰이크 시작");

    // 비동기적으로 웹소켓 핸드셰이크 수행
    ws_.async_accept(
        beast::bind_front_handler(
            &Session::on_accept,
            shared_from_this()));
}

void Session::close() {
    // 이미 종료 중이거나 종료되었으면 무시
    bool expected = false;
    if (!closing_.compare_exchange_strong(expected, true) || closed_) {
        return;
    }

    auto self = shared_from_this(); // 안전한 객체 수명 보장

    // 비동기 작업이 아닌 동기 작업으로 변경하여 완료 보장
    net::post(ws_.get_executor(), [self]() {
        try {
            if (self->ws_.is_open()) {
                beast::error_code ec;
                self->ws_.close(websocket::close_code::normal, ec);
                self->closed_ = true;
            }
        }
        catch (const std::exception& e) {
            ConsoleHelper::Error("[Error] 웹소켓 닫기 예외: " + std::string(e.what()));
        }
        });
}

bool Session::is_open() const {
    return ws_.is_open() && !closing_ && !closed_;
}

void Session::send(const std::string& message) {
    // 메시지 복사본을 만들어 비동기 작업 동안 수명 보장
    std::shared_ptr<std::string> msg_copy = std::make_shared<std::string>(message);

    net::post(
        ws_.get_executor(),
        [this, msg_copy]() {
            queue_message(*msg_copy);
        });
}

const std::string& Session::getNickname() const {
    return nickname_;
}

void Session::setNickname(const std::string& nickname) {
    nickname_ = nickname;
}

const std::string& Session::getDbUserId() const {
    return db_user_id_;
}

void Session::startPingTimer() {
    auto self = shared_from_this();
    net::post(ws_.get_executor(), [self]() {
        // 30초 후에 ping 전송
        self->ws_.async_ping(
            "",
            [self](beast::error_code ec) {
                if (!ec) {
                    self->startPingTimer();
                }
                // 실패한 경우는 무시 - 연결이 끊겼을 가능성 높음
            });
        });
}

bool Session::registerUser() {
    try {
        auto& db = DatabaseManager::getInstance();
        return db.executeTransaction([&](pqxx::work& txn) {
            // 사용자가 존재하는지 확인
            pqxx::result r = txn.exec(
                "SELECT id::text FROM Users WHERE username = " + txn.quote(nickname_)
            );

            if (r.empty()) {
                // 새 사용자 생성 - email에 더미 값 추가
                std::string dummy_email = nickname_ + "@chat.local";
                r = txn.exec(
                    "INSERT INTO Users (username, email) VALUES ("
                    + txn.quote(nickname_) + ", "
                    + txn.quote(dummy_email) + ") RETURNING id::text"
                );
                if (!r.empty()) {
                    db_user_id_ = r[0][0].as<std::string>();
                }
            }
            else {
                db_user_id_ = r[0][0].as<std::string>();
            }

            // 세션 생성
            txn.exec(
                "INSERT INTO Sessions (user_id, status) VALUES ("
                + txn.quote(db_user_id_) + "::uuid, 'active')"
            );
            });
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error("사용자 등록 실패: " + std::string(e.what()));
        return false;
    }
}

bool Session::endSession() {
    try {
        if (db_user_id_.empty()) return false;

        auto& db = DatabaseManager::getInstance();
        return db.executeTransaction([&](pqxx::work& txn) {
            txn.exec(
                "UPDATE Sessions SET status = 'ended', ended_at = CURRENT_TIMESTAMP "
                "WHERE user_id = " + txn.quote(db_user_id_) + "::uuid AND status = 'active'"
            );
            });
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error("세션 종료 기록 실패: " + std::string(e.what()));
        return false;
    }
}

void Session::queue_message(const std::string& message) {
    bool write_in_progress;

    {
        std::lock_guard<std::mutex> lock(write_mutex_);
        write_in_progress = writing_;

        // 메시지를 큐에 추가
        write_queue_.push(message);
    }

    // 현재 전송 중이 아니면 전송 시작
    if (!write_in_progress) {
        do_write();
    }
}

void Session::do_write() {
    std::shared_ptr<std::string> message_ptr;

    {
        std::lock_guard<std::mutex> lock(write_mutex_);

        // 큐가 비어있으면 종료
        if (write_queue_.empty()) {
            writing_ = false;
            return;
        }

        // 다음 메시지 가져오기
        message_ptr = std::make_shared<std::string>(write_queue_.front());
        write_queue_.pop();

        // 전송 중 상태로 설정
        writing_ = true;
    }

    // 텍스트 메시지 설정
    ws_.text(true);

    // 람다 함수를 사용하여 비동기 쓰기 작업 시작
    auto self = shared_from_this();
    ws_.async_write(
        net::buffer(*message_ptr),
        [self, message_ptr](beast::error_code ec, size_t bytes_transferred) {
            self->on_write(ec, bytes_transferred, message_ptr);
        });
}

void Session::on_write(beast::error_code ec, size_t bytes_transferred, std::shared_ptr<std::string> message_ptr) {
    boost::ignore_unused(bytes_transferred);
    boost::ignore_unused(message_ptr);  // 메시지 수명을 유지하기 위한 매개변수

    if (ec) {
        // 오류 코드를 세부적으로 분석
        std::string error_type;
        if (ec == boost::asio::error::operation_aborted) {
            error_type = "작업 취소됨";
        }
        else if (ec == boost::beast::websocket::error::closed) {
            error_type = "WebSocket 연결 닫힘";
        }
        else if (ec == boost::asio::error::eof) {
            error_type = "연결 종료 (EOF)";
        }
        else {
            // 일반 오류 메시지
            error_type = "오류: " + ec.message();
        }

        // 오류 출력 - 스레드 ID와 함께
        ConsoleHelper::Error("[Error] Write: " + error_type);

        // 쓰기 상태 초기화
        std::lock_guard<std::mutex> lock(write_mutex_);
        writing_ = false;
        return;
    }

    // 다음 메시지 처리
    do_write();
}

void Session::on_accept(beast::error_code ec) {
    if (ec) {
        ConsoleHelper::Error("[Session] WebSocket 연결 실패: " + ec.message());
        return;
    }

    ConsoleHelper::ThreadOut("[Session] WebSocket 연결 성공");

    // 사용자 DB 등록
    try {
        if (registerUser()) {
            ConsoleHelper::Out("[DB] 사용자 등록 및 세션 시작 성공: " + nickname_);
        }
        else {
            ConsoleHelper::Error("[DB] 사용자 등록 실패");
        }
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error("[DB] 사용자 등록 중 예외: " + std::string(e.what()));
    }

    // 비동기적으로 데이터 수신 대기
    do_read();
}

void Session::handle_disconnect(beast::error_code ec) {
    // 이미 종료 중이거나 종료되었으면 무시
    bool expected = false;
    if (!closing_.compare_exchange_strong(expected, true) || closed_) {
        return;
    }

    try {
        // DB에 세션 종료 기록
        try {
            if (endSession()) {
                ConsoleHelper::Out("[DB] 세션 종료 기록 성공: " + nickname_);
            }
            else {
                ConsoleHelper::Error("[DB] 세션 종료 기록 실패");
            }
        }
        catch (const std::exception& e) {
            ConsoleHelper::Error("[DB] 세션 종료 기록 중 예외: " + std::string(e.what()));
        }

        // 먼저 WebSocket 연결 종료
        if (ws_.is_open()) {
            beast::error_code close_ec;
            ws_.close(websocket::close_code::normal, close_ec);
            closed_ = true;
        }

        ConsoleHelper::Out("[Session] 클라이언트 연결 종료 완료: " + nickname_);
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error("[Error] 세션 정리 중 오류: " + std::string(e.what()));
    }
}

void Session::do_read() {
    // 이미 읽기 중이거나 종료 중이면 무시
    bool expected = false;
    if (!reading_.compare_exchange_strong(expected, true)) {
        // 이미 읽기 중이므로 여기서 중단
        ConsoleHelper::Debug("[Debug] 이미 읽기 작업 중입니다. 중복 호출 무시.");
        return;
    }

    if (closing_ || closed_) {
        // 종료 중이므로 읽기 중 상태를 초기화하고 중단
        reading_ = false;
        ConsoleHelper::Debug("[Debug] 세션이 종료 중이거나 이미 종료되었습니다. 읽기 작업 취소.");
        return;
    }

    // 명시적으로 shared_from_this()를 캡쳐하여 수명 보장
    auto self = shared_from_this();
    ws_.async_read(
        buffer_,
        [self](beast::error_code ec, size_t bytes_transferred) {
            // 읽기 작업 완료 표시 - 반드시 콜백 시작 부분에서 처리
            self->reading_ = false;

            // 콜백 수행 중 예외가 발생해도 읽기 상태는 초기화됨
            try {
                if (!ec && !self->closing_ && !self->closed_) {
                    self->on_read(ec, bytes_transferred);
                }
                else {
                    // 오류가 있거나 종료 중이면 연결 종료 처리
                    self->handle_disconnect(ec);
                }
            }
            catch (const std::exception& e) {
                ConsoleHelper::Error("[Error] 읽기 콜백 처리 중 예외: " + std::string(e.what()));
                // 예외가 발생해도 연결 종료 처리
                if (!self->closing_ && !self->closed_) {
                    self->handle_disconnect(beast::error_code());
                }
            }
        });
}

void Session::on_read(beast::error_code ec, size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    // 오류 처리
    if (ec) {
        // 간단히 연결 종료 메시지만 표시 (오류 상세 정보 제외)
        ConsoleHelper::Out("[Session] 클라이언트 연결 종료: " + nickname_);
        return;
    }

    // 수신한 메시지 처리
    std::string message = beast::buffers_to_string(buffer_.data());
    ConsoleHelper::Out("[Recv] " + nickname_ + ": " + message);

    try {
        // 명령어 처리
        if (!processCommand(message)) {
            // 일반 채팅 메시지
            auto chat_msg = ChatMessage::createChatMessage(nickname_, message);
            // 메시지 브로드캐스트는 ChatRoom이 처리
        }
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error("[Error] 메시지 처리 오류: " + std::string(e.what()));

        // 오류 메시지 전송
        auto error_msg = ChatMessage::createSystemMessage("메시지 처리 중 오류가 발생했습니다.");
        send(error_msg.toJson());
    }

    // 버퍼 비우기
    buffer_.consume(buffer_.size());

    // 다음 메시지 읽기
    do_read();
}

bool Session::processCommand(const std::string& message) {
    // 명령어 처리 - 선행 슬래시(/)로 시작하는지 확인
    if (message.length() > 1 && message[0] == '/') {
        std::string cmd;
        std::string args;

        // 공백으로 명령어와 인자 분리
        size_t spacePos = message.find(' ');
        if (spacePos != std::string::npos) {
            cmd = message.substr(1, spacePos - 1);
            args = message.substr(spacePos + 1);
            // 앞뒤 공백 제거
            args = StringUtils::Trim(args);
        }
        else {
            cmd = message.substr(1);
            args = "";
        }

        // 대소문자 구분 없이 명령어 처리
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), [](unsigned char c) { return std::tolower(c); });

        if (cmd == "nick") {
            handleNicknameChange(args);
            return true;
        }
        else if (cmd == "whisper" || cmd == "w") {
            handleWhisperCommand(args);
            return true;
        }
        else if (cmd == "help") {
            // 도움말 메시지 생성
            std::string helpMsg =
                "사용 가능한 명령어:\n"
                "/nick <새닉네임> - 닉네임 변경\n"
                "/whisper <대상> <메시지> - 귓속말 전송 (/w로도 가능)\n"
                "/help - 도움말 표시";

            auto systemMsg = ChatMessage::createSystemMessage(helpMsg);
            send(systemMsg.toJson());
            return true;
        }
    }

    // 명령어가 아닌 경우
    return false;
}

void Session::handleNicknameChange(const std::string& new_nickname) {
    // 닉네임 유효성 검사
    if (new_nickname.empty() || new_nickname.length() > 20) {
        auto error_msg = ChatMessage::createSystemMessage("닉네임은 1~20자 사이여야 합니다.");
        send(error_msg.toJson());
        return;
    }

    std::string old_nickname = nickname_;

    // DB에 닉네임 변경 기록
    try {
        if (!db_user_id_.empty()) {
            auto& db = DatabaseManager::getInstance();
            db.executeTransaction([&](pqxx::work& txn) {
                txn.exec(
                    "UPDATE Users SET username = " + txn.quote(new_nickname) +
                    " WHERE id = " + txn.quote(db_user_id_) + "::uuid"
                );
                });
        }

        // 닉네임 변경
        nickname_ = new_nickname;

        // 닉네임 변경 알림
        auto system_msg = ChatMessage::createSystemMessage(
            old_nickname + "님이 " + nickname_ + "으로 닉네임을 변경했습니다.");

        // ChatRoom에 알림
        // 여기서는 직접 broadcast 호출이 아닌, 외부 콜백으로 처리

        ConsoleHelper::Out("[Session] 닉네임 변경: " + old_nickname + " -> " + nickname_);

        // 성공 메시지
        auto success_msg = ChatMessage::createSystemMessage("닉네임이 " + nickname_ + "으로 변경되었습니다.");
        send(success_msg.toJson());
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error("[DB] 닉네임 변경 실패: " + std::string(e.what()));

        // 오류 메시지 전송
        auto error_msg = ChatMessage::createSystemMessage("닉네임 변경 중 오류가 발생했습니다.");
        send(error_msg.toJson());
    }
}

void Session::handleWhisperCommand(const std::string& target_and_message) {
    // 대상과 메시지 분리
    size_t spacePos = target_and_message.find(' ');
    if (spacePos == std::string::npos || spacePos == 0) {
        // 형식 오류
        auto error_msg = ChatMessage::createSystemMessage("올바른 형식: /whisper <대상닉네임> <메시지>");
        send(error_msg.toJson());
        return;
    }

    std::string target = target_and_message.substr(0, spacePos);
    std::string whisper_message = target_and_message.substr(spacePos + 1);

    // 귓속말 처리는 ChatRoom에서 담당
    // 여기서는 직접 whisper 호출이 아닌, 외부 콜백으로 처리

    // 템플릿 코드이므로 실제 귓속말 전송 로직은 ChatServerImpl에서 구현
    ConsoleHelper::Out("[Session] 귓속말 요청: " + nickname_ + " -> " + target + ": " + whisper_message);
}
