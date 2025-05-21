// main.cpp
#include <iostream>
#include <string>
#include <Windows.h>
#include <memory>
#include <csignal>
#include <thread>
#include <atomic>

#include "utils/ConsoleHelper.h"
#include "config/ServerConfig.h"
#include "IChatServer.h"

std::atomic<bool> g_running = true;

// 시그널 핸들러
void signalHandler(int signal) {
    ConsoleHelper::OutWithColor("서버 종료 요청 수신...", ConsoleColor::Yellow);
    g_running = false;
}

// 로그 콜백 함수
void logCallback(const std::string& message) {
    ConsoleHelper::Out(message);
}

int main(int argc, char* argv[]) {
    try {
        // 콘솔 초기화
        ConsoleHelper::Initialize();
        ConsoleHelper::OutWithColor("채팅 서버 시작 중...", ConsoleColor::Green);

        // 시그널 핸들러 설정
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);

        // 서버 설정 로드
        ServerConfig config;
        std::string configFile = "server_config.ini";

        // 명령행 인수가 있으면 설정 파일 경로 지정
        if (argc > 1) {
            configFile = argv[1];
        }

        // 설정 파일 로드 시도
        if (config.loadFromFile(configFile)) {
            ConsoleHelper::Out("설정 파일 로드 성공: " + configFile);
        }
        else {
            ConsoleHelper::OutWithColor("설정 파일 로드 실패, 기본 설정 사용: " + configFile, ConsoleColor::Yellow);
        }

        // 설정 정보 출력
        ConsoleHelper::Out(config.toString());

        // 서버 인스턴스 생성
        std::unique_ptr<IChatServer> server = CreateChatServer(config);

        // 로그 콜백 설정
        server->SetLogCallback(logCallback);

        // 서버 초기화
        if (!server->Initialize()) {
            ConsoleHelper::OutWithColor("서버 초기화 실패", ConsoleColor::Red);
            return 1;
        }

        // 서버 시작
        if (!server->Start()) {
            ConsoleHelper::OutWithColor("서버 시작 실패", ConsoleColor::Red);
            return 1;
        }

        ConsoleHelper::OutWithColor("채팅 서버가 시작되었습니다.", ConsoleColor::Green);
        ConsoleHelper::Out("주소: " + server->GetServerAddress() + ":" + std::to_string(server->GetServerPort()));
        ConsoleHelper::Out("Ctrl+C를 눌러 서버를 종료할 수 있습니다.");

        // 메인 스레드 유지
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            // 주기적으로 서버 상태 정보 출력 (예: 10초마다)
            static int counter = 0;
            if (++counter % 10 == 0) {
                ConsoleHelper::Out("연결된 클라이언트 수: " + std::to_string(server->GetConnectionCount()));
                ConsoleHelper::Out(server->GetServerStats());
            }
        }

        // 서버 종료
        ConsoleHelper::OutWithColor("서버를 종료합니다...", ConsoleColor::Yellow);
        server->Stop();
        ConsoleHelper::OutWithColor("서버가 종료되었습니다.", ConsoleColor::Green);

    }
    catch (const std::exception& e) {
        ConsoleHelper::OutWithColor("예외 발생: " + std::string(e.what()), ConsoleColor::Red);
        return 1;
    }

    return 0;
}
