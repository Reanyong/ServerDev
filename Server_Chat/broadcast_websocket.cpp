// **#**
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <Windows.h>
#include <io.h>
#include <fcntl.h>
#include <string>
#include <thread>
#include <vector>
#include <set>
#include <mutex>
#include <queue>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;
using json = nlohmann::json; // JSON 파싱을 위한 라이브러리
using namespace std;

// UTF-8 문자열을 와이드 문자열로 변환
wstring utf8_to_wstring(const string& str) {
    if (str.empty()) return wstring();

    try {
        // 필요한 버퍼 크기 계산
        int size_need = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
        if (size_need <= 0) {
            // 변환 실패 시 오류 내용을 직접 확인
            DWORD error = GetLastError();
            return L"<변환 오류: " + to_wstring(error) + L">";
        }

        // 버퍼 할당 및 변환
        wstring result(size_need, 0);
        if (MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &result[0], size_need) <= 0) {
            DWORD error = GetLastError();
            return L"<변환 오류: " + to_wstring(error) + L">";
        }

        return result;
    }
    catch (const std::exception& e) {
        // 예외 발생 시 안전한 문자열 반환
        string what_str = e.what(); // 문자열로 변환
        wstring what_wstr;

        // 간단히 문자 단위로 변환 (ASCII 문자만 정상 처리됨)
        for (char c : what_str) {
            what_wstr.push_back(static_cast<wchar_t>(c));
        }

        return L"<예외 발생: " + what_wstr + L">";
    }
}

// 콘솔에 메시지 출력
void ConsoleOut(const wstring& message) {
    static mutex console_mutex;
    lock_guard<mutex> lock(console_mutex);  // 스레드 안전성을 위해 뮤텍스 사용

    try {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwWritten = 0;

        // 콘솔 출력 API 사용
        WriteConsoleW(hConsole, message.c_str(), (DWORD)message.length(), &dwWritten, NULL);
        WriteConsoleW(hConsole, L"\r\n", 2, &dwWritten, NULL);
    }
    catch (const std::exception& e) {
        // 출력 실패 시 대체 방법 시도
        std::wcerr << L"출력 오류: " << e.what() << std::endl;
        std::wcout << message << std::endl;
    }
}


// 콘솔에 에러 메시지 출력
void ConsoleErr(const wstring& message) {
    static mutex console_mutex;
    lock_guard<mutex> lock(console_mutex);  // 스레드 안전성을 위해 뮤텍스 사용

    try {
        HANDLE hConsole = GetStdHandle(STD_ERROR_HANDLE);
        DWORD dwWritten = 0;

        // 콘솔 출력 API 사용
        WriteConsoleW(hConsole, message.c_str(), (DWORD)message.length(), &dwWritten, NULL);
        WriteConsoleW(hConsole, L"\r\n", 2, &dwWritten, NULL);
    }
    catch (const std::exception& e) {
        // 출력 실패 시 대체 방법 시도
        std::wcerr << L"오류 출력 실패: " << e.what() << std::endl;
        std::wcerr << message << std::endl;
    }
}

#ifdef _DEBUG
void print_buffer_info(const string& message, const char* func_name) {
    ConsoleOut(utf8_to_wstring(string(func_name) + ": Buffer size=" +
        to_string(message.size()) + ", capacity=" +
        to_string(message.capacity())));
}
#else
#define print_buffer_info(msg, func) ((void)0)
#endif

// 메세지 타입 정의
enum class MessageType {
	JOIN,       // 사용자 입장
	LEAVE,      // 사용자 퇴장
	CHAT,       // 일반 채팅
	SYSTEM,     // 시스템 메시지
};

// 채팅 구조체
struct ChatMessage {
	MessageType type;       // 메시지 타입
	string nickname;        // 사용자 이름
	string content;         // 메시지 내용
	string timestamp;       // 타임스탬프

    // json 직렬화
	string toJson() const {
		json j;
		j["type"] = static_cast<int>(type);
		j["nickname"] = nickname;
		j["content"] = content;
		j["timestamp"] = timestamp;
		return j.dump();
	}

	// json 역직렬화
	static ChatMessage fromJson(const string& jsonString) {
		ChatMessage msg;
		json j = json::parse(jsonString);
		msg.type = static_cast<MessageType>(j["type"].get<int>());
		msg.nickname = j["nickname"].get<string>();
		msg.content = j["content"].get<string>();
		msg.timestamp = j["timestamp"].get<string>();
		return msg;
	}

    // 메세지 생성 헬퍼 함수
    static ChatMessage createJoinMessage(const string& nickname) {
        return { MessageType::JOIN, nickname, nickname + "님이 입장하셨습니다.", getCurrentTimestamp() };
    }

	static ChatMessage createLeaveMessage(const string& nickname) {
		return { MessageType::LEAVE, nickname, nickname + "님이 퇴장하셨습니다.", getCurrentTimestamp() };
	}

	static ChatMessage createChatMessage(const string& nickname, const string& content) {
		return { MessageType::CHAT, nickname, content, getCurrentTimestamp() };
	}

	static ChatMessage createSystemMessage(const string& content) {
		return { MessageType::SYSTEM, "System", content, getCurrentTimestamp() };
	}

private:
    static string getCurrentTimestamp()
    {
		auto now = chrono::system_clock::now();
		auto in_time_t = chrono::system_clock::to_time_t(now);

        stringstream ss;
		tm timeinfo;
		localtime_s(&timeinfo, &in_time_t);
		ss << put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
		return ss.str();
    }
};

// Session 클래스 전방 선언
class Session;

class ChatRoom
{
private:
    set<shared_ptr<Session>> sessions_; // 참가자 목록
    mutex mutex_;                       // 참가자 목록 접근을 위한 뮤텍스
    int next_user_id_ = 1;

public:
    // 채팅방 세션 추가
    void join(shared_ptr<Session> session);

    // 채팅방 세션 제거
    void leave(shared_ptr<Session> session);

    // 모든 세션 메세지 브로드캐스트
    void broadcast(const string& message, shared_ptr<Session> sender = nullptr);

    int generateUserId() {
        lock_guard<mutex> lock(mutex_);
        return next_user_id_++;
    }

    size_t getSeesionCount() {
        lock_guard<mutex> lock(mutex_);
        return sessions_.size();
    }


};

// 전역 채팅방 인스턴스
ChatRoom g_chatRoom;

// 세션 클래스
class Session : public enable_shared_from_this<Session>
{
private:
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;
    mutex write_mutex_;
    mutex read_mutex_;              // 읽기 작업 동기화를 위한 뮤텍스 추가
    atomic<bool> reading_{ false };   // 읽기 작업 상태 플래그
    atomic<bool> writing_{ false };   // 쓰기 작업 상태 플래그
    atomic<bool> closing_{ false };   // 종료 중 플래그 추가
    atomic<bool> closed_{ false };    // 이미 종료됨 플래그 추가
    string nickname_;
    int user_id_;
    queue<string> write_queue_;

public:
    // 소켓을 받아 세션 생성
    explicit Session(tcp::socket socket)
        : ws_(move(socket)) {
        ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
        user_id_ = g_chatRoom.generateUserId();
        nickname_ = "User" + to_string(user_id_);
    }

    ~Session() {
        try {
            // 이미 closed_ 플래그가 설정되어 있지 않다면 명시적으로 close 호출
            if (!closed_) {
                beast::error_code ec;
                ws_.close(websocket::close_code::normal, ec);
            }
        }
        catch (...) {
            // 소멸자에서는 예외 무시
        }
    }

    void start_ping_timer() {
        auto self = shared_from_this();
        net::post(ws_.get_executor(), [self]() {
            // 30초 후에 ping 전송
            self->ws_.async_ping(
                "",
                [self](beast::error_code ec) {
                    if (!ec) {
                        self->start_ping_timer();
                    }
                    // 실패한 경우는 무시 - 연결이 끊겼을 가능성 높음
                });
            });
    }

    void close() {
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
                ConsoleErr(L"[Error] 웹소켓 닫기 예외: " + utf8_to_wstring(e.what()));
            }
            });
    }

    void start() {
        // 현재 스레드 ID 출력
        ConsoleOut(L"[Session] 웹소켓 핸드셰이크 시작 [Thread ID: " +
            to_wstring(hash<thread::id>{}(this_thread::get_id())) + L"]");

        // 비동기적으로 웹소켓 핸드셰이크 수행
        ws_.async_accept(
            beast::bind_front_handler(
                &Session::on_accept,
                shared_from_this()));
    }

    // 메시지 전송 (다른 세션에서 호출)
    void send(const string& message) {
        // 메시지 복사본을 만들어 비동기 작업 동안 수명 보장
        shared_ptr<string> msg_copy = make_shared<string>(message);

        net::post(
            ws_.get_executor(),
            [this, msg_copy]() {
                queue_message(*msg_copy);
            });
    }

    // 닉네임 가져오기
    const string& getNickname() const {
        return nickname_;
    }

    bool is_open() const {
        return ws_.is_open();
    }

private:
    // 메세지 큐 생성 -> 필요시 전송
    void queue_message(const string& message) {
        bool write_in_progress;

        {
            lock_guard<mutex> lock(write_mutex_);
            write_in_progress = writing_;

            // 메시지를 큐에 추가
            write_queue_.push(message);
        }

        // 현재 전송 중이 아니면 전송 시작
        if (!write_in_progress) {
            do_write();
        }
    }

    // 큐에서 다음 메시지를 가져와 전송 시작
    void do_write() {
        shared_ptr<string> message_ptr;

        {
            lock_guard<mutex> lock(write_mutex_);

            // 큐가 비어있으면 종료
            if (write_queue_.empty()) {
                writing_ = false;
                return;
            }

            // 다음 메시지 가져오기
            message_ptr = make_shared<string>(write_queue_.front());
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

    /*
    // 메시지 전송 내부 구현
    void do_send(const string& message) {
        lock_guard<mutex> lock(write_mutex_);

        // 큐를 사용하여 메시지를 저장하고 순차적으로 전송하는 방식으로 확장 가능

        // 텍스트 메시지 설정
        ws_.text(true);

        // 비동기 쓰기 작업 시작
        ws_.async_write(
            net::buffer(message),
            beast::bind_front_handler(
                &Session::on_write,
                shared_from_this()));
    }
    */

    void on_write(beast::error_code ec, size_t bytes_transferred, shared_ptr<string> message_ptr) {
        boost::ignore_unused(bytes_transferred);
        boost::ignore_unused(message_ptr);  // 메시지 수명을 유지하기 위한 매개변수

        if (ec) {
            // 오류 코드를 세부적으로 분석
            wstring error_type;
            if (ec == boost::asio::error::operation_aborted) {
                error_type = L"작업 취소됨";
            }
            else if (ec == boost::beast::websocket::error::closed) {
                error_type = L"WebSocket 연결 닫힘";
            }
            else if (ec == boost::asio::error::eof) {
                error_type = L"연결 종료 (EOF)";
            }
            else {
                // 일반 오류 메시지
                error_type = L"오류: " + utf8_to_wstring(ec.message());
            }

            // 오류 출력 - 스레드 ID와 함께
            ConsoleErr(L"[Error] Write: " + error_type +
                L" [Thread ID: " + to_wstring(hash<thread::id>{}(this_thread::get_id())) + L"]");

            // 쓰기 상태 초기화
            lock_guard<mutex> lock(write_mutex_);
            writing_ = false;
            return;
        }

        // 다음 메시지 처리
        do_write();
    }

    // 웹소켓 연결 후 콜백
    void on_accept(beast::error_code ec)
    {
        if (ec) {
            ConsoleErr(L"[Session] WebSocket 연결 실패: " + utf8_to_wstring(ec.message()));
            return;
        }

        ConsoleOut(L"[Session] WebSocket 연결 성공 [Thread ID: " +
            to_wstring(hash<thread::id>{}(this_thread::get_id())) + L"]");

        // 채팅방 세션 추가
        g_chatRoom.join(shared_from_this());

        // 입장 메시지 브로드캐스트
        auto join_msg = ChatMessage::createJoinMessage(nickname_);
        g_chatRoom.broadcast(join_msg.toJson());

        // Welcome 메세지 전송
        auto welcome_msg = ChatMessage::createSystemMessage("환영합니다! 현재 " + to_string(g_chatRoom.getSeesionCount()) + "명이 접속 중입니다.");
        send(welcome_msg.toJson());

        // 비동기적으로 데이터 수신 대기
        do_read();
    }

    void handle_disconnect(beast::error_code ec) {
        // 이미 종료 중이거나 종료되었으면 무시
        bool expected = false;
        if (!closing_.compare_exchange_strong(expected, true) || closed_) {
            return;
        }

        try {
            // 먼저 WebSocket 연결 종료
            if (ws_.is_open()) {
                beast::error_code close_ec;
                ws_.close(websocket::close_code::normal, close_ec);
                closed_ = true;
            }

            // 퇴장 메시지 브로드캐스트 
            auto leave_msg = ChatMessage::createLeaveMessage(nickname_);
            g_chatRoom.broadcast(leave_msg.toJson(), shared_from_this());

            // 채팅방에서 제거
            g_chatRoom.leave(shared_from_this());

            ConsoleOut(L"[Session] 클라이언트 연결 종료 완료: " + utf8_to_wstring(nickname_));
        }
        catch (const std::exception& e) {
            ConsoleErr(L"[Error] 세션 정리 중 오류: " + utf8_to_wstring(e.what()));
        }
    }

    // 비동기적으로 메시지 읽기
    void do_read() {
        // 이미 읽기 중이거나 종료 중이면 무시
        bool expected = false;
        if (!reading_.compare_exchange_strong(expected, true)) {
            // 이미 읽기 중이므로 여기서 중단
            ConsoleOut(L"[Debug] 이미 읽기 작업 중입니다. 중복 호출 무시.");
            return;
        }

        if (closing_ || closed_) {
            // 종료 중이므로 읽기 중 상태를 초기화하고 중단
            reading_ = false;
            ConsoleOut(L"[Debug] 세션이 종료 중이거나 이미 종료되었습니다. 읽기 작업 취소.");
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
                    ConsoleErr(L"[Error] 읽기 콜백 처리 중 예외: " + utf8_to_wstring(e.what()));
                    // 예외가 발생해도 연결 종료 처리
                    if (!self->closing_ && !self->closed_) {
                        self->handle_disconnect(beast::error_code());
                    }
                }
            });
    }

    // 메시지 읽기 완료 후 호출되는 콜백
    void on_read(beast::error_code ec, size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        // 오류 처리
        if (ec) {
            // 간단히 연결 종료 메시지만 표시 (오류 상세 정보 제외)
            ConsoleOut(L"[Session] 클라이언트 연결 종료: " + utf8_to_wstring(nickname_));

            try {
                // 퇴장 메시지 브로드캐스트
                auto leave_msg = ChatMessage::createLeaveMessage(nickname_);
                g_chatRoom.broadcast(leave_msg.toJson(), shared_from_this());

                // 채팅방에서 제거
                g_chatRoom.leave(shared_from_this());
            }
            catch (const std::exception& e) {
                ConsoleErr(L"[Error] 세션 정리 중 오류: " + utf8_to_wstring(e.what()));
            }

            return;
        }

        // 수신한 메시지 처리
        string message = beast::buffers_to_string(buffer_.data());
        ConsoleOut(L"[Recv] " + utf8_to_wstring(nickname_ + ": " + message));

        // 여기에 디버깅 코드 삽입 - 메시지 내용을 바이트 단위로 확인
        wstring debug_msg = L"[Raw bytes]: ";
        for (size_t i = 0; i < min(message.size(), size_t(20)); ++i) {
            debug_msg += to_wstring(static_cast<unsigned char>(message[i])) + L" ";
        }
        ConsoleOut(debug_msg);

        try {
            // 메시지 처리 (닉네임 변경 명령어 처리 또는 일반 채팅)
            if (message.length() >= 6 && message.substr(0, 6) == "/nick ") {
                // 닉네임 변경 명령어 (앞뒤 공백 제거)
                string new_nickname = message.substr(6);
                // 앞뒤 공백 제거
                new_nickname.erase(0, new_nickname.find_first_not_of(" \t\r\n"));
                new_nickname.erase(new_nickname.find_last_not_of(" \t\r\n") + 1);

                string old_nickname = nickname_;

                // 닉네임 로깅
                ConsoleOut(L"[닉네임 변경 시도] '" + utf8_to_wstring(old_nickname) +
                    L"' -> '" + utf8_to_wstring(new_nickname) + L"'");

                // 닉네임 유효성 검사
                if (new_nickname.empty() || new_nickname.length() > 20) {
                    auto error_msg = ChatMessage::createSystemMessage("닉네임은 1~20자 사이여야 합니다.");
                    send(error_msg.toJson());
                }
                else {
                    nickname_ = new_nickname;

                    // 닉네임 변경 알림
                    auto system_msg = ChatMessage::createSystemMessage(
                        old_nickname + "님이 " + nickname_ + "으로 닉네임을 변경했습니다.");
                    g_chatRoom.broadcast(system_msg.toJson());
                }
            }
            else {
                // 일반 채팅 메시지
                auto chat_msg = ChatMessage::createChatMessage(nickname_, message);
                g_chatRoom.broadcast(chat_msg.toJson());
            }
        }
        catch (const exception& e) {
            ConsoleErr(L"[Error] 메시지 처리 오류: " + utf8_to_wstring(e.what()));

            // 오류 메시지 전송
            auto error_msg = ChatMessage::createSystemMessage("메시지 처리 중 오류가 발생했습니다.");
            send(error_msg.toJson());
        }

        // 버퍼 비우기
        buffer_.consume(buffer_.size());

        // 다음 메시지 읽기
        do_read();

        /*
        lock_guard<mutex> lock(write_mutex_);  // 쓰기 작업 동기화

        // 에코 응답 (비동기적으로 메시지 전송)
        ws_.text(ws_.got_text());
        ws_.async_write(
            buffer_.data(),
            beast::bind_front_handler(
                &Session::on_write,
                shared_from_this()));
        */
    }
};

void ChatRoom::join(shared_ptr<Session> session) {
    lock_guard<mutex> lock(mutex_);
    sessions_.insert(session);
}

void ChatRoom::leave(shared_ptr<Session> session) {
    lock_guard<mutex> lock(mutex_);
    sessions_.erase(session);
}

void ChatRoom::broadcast(const string& message, shared_ptr<Session> sender) {
    vector<weak_ptr<Session>> targets;
    {
        lock_guard<mutex> lock(mutex_);
        for (auto& session : sessions_) {
            if (session != sender) {
                targets.push_back(session);
            }
        }
    }

    // 잠금 없이 안전하게 메시지 전송
    for (auto& weak_session : targets) {
        if (auto session = weak_session.lock()) {
            try {
                // 소켓이 열려있는지 확인 
                if (!session->is_open()) continue;
                session->send(message);
            }
            catch (const std::exception& e) {
                ConsoleErr(L"[Error] 메시지 전송 중 예외: " + utf8_to_wstring(e.what()));
            }
        }
    }
}

class Server {
private:
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    vector<thread> threads_;
    atomic<int> next_strand_index_{ 0 };  // 라운드 로빈 방식으로 strand 할당에 사용할 인덱스
    vector<net::strand<net::io_context::executor_type>> strands_;  // 여러 strand 생성

public:
    Server(net::io_context& ioc, const tcp::endpoint& endpoint, int thread_count)
        : ioc_(ioc),
        acceptor_(ioc, endpoint) {
        // 멀티스레드 처리를 위한 strand 생성
        // strand는 각 스레드 간 작업을 분산시키는 역할
        int num_threads = max(thread_count, 1);
        strands_.reserve(num_threads);

        for (int i = 0; i < num_threads; ++i) {
            strands_.emplace_back(net::make_strand(ioc));
        }

        ConsoleOut(L"[Server] " + to_wstring(num_threads) +
            L"개의 스레드와 " + to_wstring(strands_.size()) +
            L"개의 strand로 서버 시작");

        do_accept();
    }

    // 서버 스레드 시작
    void run() {
        // 하드웨어 코어 수 또는 지정된 값으로 최적의 스레드 수 결정
        int num_threads = thread::hardware_concurrency();

        // 스레드 생성 및 실행
        threads_.reserve(num_threads);

        for (int i = 0; i < num_threads; ++i) {
            threads_.emplace_back([this, i] {
                ConsoleOut(L"[Thread] 작업 스레드 #" + to_wstring(i) + L" 시작 [ID: " +
                    to_wstring(hash<thread::id>{}(this_thread::get_id())) + L"]");

                // 각 스레드가 io_context에서 작업을 처리
                this->ioc_.run();

                ConsoleOut(L"[Thread] 작업 스레드 #" + to_wstring(i) + L" 종료");
                });
        }

        // 모든 스레드가 종료될 때까지 대기
        for (auto& thread : threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

private:
    // 클라이언트 연결 수락
    void do_accept() {
        // 다음 연결은 다른 strand에서 처리하도록 설정
        int strand_idx = next_strand_index_++;
        if (next_strand_index_ >= strands_.size()) {
            next_strand_index_ = 0;  // 라운드 로빈 방식
        }

        // 해당 strand에서 비동기 수락 작업 진행
        acceptor_.async_accept(
            net::bind_executor(
                strands_[strand_idx],
                [this, strand_idx](beast::error_code ec, tcp::socket socket) {
                    if (!ec) {
                        ConsoleOut(L"[Server] 클라이언트 연결됨 [Thread ID: " +
                            to_wstring(hash<thread::id>{}(this_thread::get_id())) +
                            L", Strand: " + to_wstring(strand_idx) + L"]");

                        try {
                            // 다른 strand에서 세션 시작
                            int session_strand_idx = next_strand_index_++;
                            if (next_strand_index_ >= strands_.size()) {
                                next_strand_index_ = 0;
                            }

                            // 세션 생성
                            auto session = make_shared<Session>(move(socket));

                            // 다른 strand에서 세션 시작 작업 포스팅
                            net::post(
                                strands_[session_strand_idx],
                                [this, session, session_strand_idx]() {
                                    try {
                                        if (!session->is_open()) {
                                            ConsoleErr(L"[Error] 세션이 이미 닫혔습니다.");
                                            return;
                                        }

                                        ConsoleOut(L"[Session] 세션 시작 [Thread ID: " +
                                            to_wstring(hash<thread::id>{}(this_thread::get_id())) +
                                            L", Strand: " + to_wstring(session_strand_idx) + L"]");

                                        // 명시적으로 세션의 시작 함수를 호출
                                        session->start();
                                    }
                                    catch (const std::exception& e) {
                                        ConsoleErr(L"[Error] 세션 시작 중 예외: " + utf8_to_wstring(e.what()));
                                    }
                                });
                        }
                        catch (const std::exception& e) {
                            ConsoleErr(L"[Error] 클라이언트 연결 처리 중 예외: " + utf8_to_wstring(e.what()));
                        }
                    }
                    else {
                        ConsoleErr(L"[Error] Accept: " + utf8_to_wstring(ec.message()));
                    }

                    // 다음 클라이언트 연결 대기 - 다른 strand에서 처리
                    // 여기에서 새로운 strand 인덱스를 사용하여 분산 처리
                    int next_accept_strand_idx = next_strand_index_++;
                    if (next_strand_index_ >= strands_.size()) {
                        next_strand_index_ = 0;
                    }

                    // 다음 accept 작업을 다른 strand에 포스팅
                    net::post(
                        strands_[next_accept_strand_idx],
                        [this]() {
                            do_accept();
                        });
                }));
    }
};


int main() {
    try {
        // 콘솔 설정
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        setlocale(LC_ALL, "ko_KR.UTF-8"); // 로케일 설정

        // 시작 메시지
        ConsoleOut(L"채팅 서버 시작 (WebSocket) - 포트 8080");
        ConsoleOut(L"명령어 안내: /nick [새닉네임] - 닉네임 변경");

        int thread_count = thread::hardware_concurrency(); // 하드웨어 코어 수 정보 출력
        ConsoleOut(L"시스템 CPU 코어/스레드 수: " + to_wstring(thread_count));

        // IO 컨텍스트 생성 - 각 스레드가 작업을 할당받을 컨텍스트
        net::io_context ioc{ thread_count };
        //ioc.get_executor().set_concurrency_limit(1000);  // 작업 큐 크기 제한

        // 서버 인스턴스 생성 및 시작
        Server server(ioc, tcp::endpoint(tcp::v4(), 8080), thread_count);

        // 서버 실행
        server.run();
    }
    catch (const exception& e) {
        wstring error_msg = L"예외 발생: ";
        wstring what_msg;

        try {
            // e.what()을 wstring으로 변환 시도
            what_msg = utf8_to_wstring(e.what());
        }
        catch (...) {
            // 변환 실패 시 안전한 대체 텍스트
            what_msg = L"<메시지 변환 실패>";
        }

        ConsoleErr(error_msg + what_msg);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
// **#**
