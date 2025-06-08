// main.cpp
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <iostream>
#include <memory>
#include <csignal>
#include <thread>
#include <atomic>

#include "config/ApiConfig.h"
#include "net/HttpServer.h"
#include "../Server_Chat/utils/ConsoleHelper.h"

std::atomic<bool> g_running = true;
std::unique_ptr<HttpServer> g_server;

// Windows 콘솔 이벤트 핸들러
BOOL WINAPI ConsoleHandler(DWORD dwCtrlType) {
    switch (dwCtrlType) {
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
        ConsoleHelper::OutWithColor("종료 신호 수신...", ConsoleColor::Yellow);
        g_running = false;

        if (g_server) {
            g_server->stop();
        }

        return TRUE;
    default:
        return FALSE;
    }
}

int main() {
    try {
        // 콘솔 초기화
        ConsoleHelper::Initialize();
        ConsoleHelper::OutWithColor("API 서버 시작 중...", ConsoleColor::Green);

        // Windows 콘솔 핸들러 설정
        SetConsoleCtrlHandler(ConsoleHandler, TRUE);

        // 서버 설정
        ApiConfig config;
        ConsoleHelper::Out(config.toString());

        // 서버 생성 및 시작
        g_server = std::make_unique<HttpServer>(config);

        if (!g_server->start()) {
            ConsoleHelper::OutWithColor("서버 시작 실패", ConsoleColor::Red);
            return 1;
        }

        ConsoleHelper::OutWithColor("API 서버가 시작되었습니다!", ConsoleColor::Green);
        ConsoleHelper::Out("접속 주소: " + g_server->getAddress());
        ConsoleHelper::Out("테스트 URL:");
        ConsoleHelper::Out("  - " + g_server->getAddress() + "/health");
        ConsoleHelper::Out("  - " + g_server->getAddress() + "/api/test");
        ConsoleHelper::Out("Ctrl+C를 눌러 서버를 종료할 수 있습니다.");

        // 메인 루프
        while (g_running && g_server->isRunning()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // 서버 종료
        if (g_server) {
            g_server->stop();
            g_server.reset();
        }

        ConsoleHelper::OutWithColor("API 서버가 종료되었습니다.", ConsoleColor::Green);
        SetConsoleCtrlHandler(ConsoleHandler, FALSE);

    }
    catch (const std::exception& e) {
        ConsoleHelper::OutWithColor("예외 발생: " + std::string(e.what()), ConsoleColor::Red);
        return 1;
    }

    return 0;
}
