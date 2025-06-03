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
std::unique_ptr<IChatServer> g_server;

// Windows 콘솔 이벤트 핸들러
BOOL WINAPI ConsoleHandler(DWORD dwCtrlType) {
    switch (dwCtrlType) {
        case CTRL_C_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            ConsoleHelper::OutWithColor("Windows 종료 이벤트 수신됨...", ConsoleColor::Yellow);
            g_running = false;
            
            // 서버가 존재하면 즉시 정지 시작
            if (g_server) {
                ConsoleHelper::OutWithColor("서버 정지 시작...", ConsoleColor::Yellow);
                try {
                    g_server->Stop();
                    ConsoleHelper::OutWithColor("서버 정지 완료", ConsoleColor::Green);
                }
                catch (const std::runtime_error& e) {
                    ConsoleHelper::OutWithColor("서버 정지 중 runtime_error: " + std::string(e.what()), ConsoleColor::Red);
                }
                catch (const std::exception& e) {
                    ConsoleHelper::OutWithColor("서버 정지 중 예외 발생: " + std::string(e.what()), ConsoleColor::Red);
                }
                catch (...) {
                    ConsoleHelper::OutWithColor("서버 정지 중 알 수 없는 예외 발생", ConsoleColor::Red);
                }
                
                // 정리 시간을 줌
                Sleep(500);
            }
            
            return TRUE;  // 이벤트 처리 완료
        default:
            return FALSE;  // 다른 핸들러가 처리하도록 함
    }
}

// 시그널 핸들러 (추가 보호)
void signalHandler(int signal) {
    ConsoleHelper::OutWithColor("POSIX 시그널 수신...", ConsoleColor::Yellow);
    g_running = false;
    
    // 서버가 존재하면 즉시 정지 시작
    if (g_server) {
        ConsoleHelper::OutWithColor("서버 정지 시작...", ConsoleColor::Yellow);
        try {
            g_server->Stop();
        }
        catch (const std::exception& e) {
            ConsoleHelper::OutWithColor("서버 정지 중 예외 발생: " + std::string(e.what()), ConsoleColor::Red);
        }
    }
}

// 로그 콜백 함수
void logCallback(const std::string& message) {
    ConsoleHelper::Out(message);
}

int main(int argc, char* argv[]) {
    try {
        // 콘솔 초기화 강화
        ConsoleHelper::Initialize();
        
        // 추가 UTF-8 설정 (Release 모드 대응)
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        
        ConsoleHelper::OutWithColor("채팅 서버 시작 중...", ConsoleColor::Green);

        // Windows 콘솔 핸들러 설정
        if (!SetConsoleCtrlHandler(ConsoleHandler, TRUE)) {
            ConsoleHelper::OutWithColor("Windows 콘솔 핸들러 설정 실패", ConsoleColor::Red);
        }

        // POSIX 시그널 핸들러 설정 (추가 보호)
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
        g_server = CreateChatServer(config);

        // 로그 콜백 설정
        g_server->SetLogCallback(logCallback);

        // 서버 초기화
        if (!g_server->Initialize()) {
            ConsoleHelper::OutWithColor("서버 초기화 실패", ConsoleColor::Red);
            return 1;
        }

        // 서버 시작
        if (!g_server->Start()) {
            ConsoleHelper::OutWithColor("서버 시작 실패", ConsoleColor::Red);
            return 1;
        }

        ConsoleHelper::OutWithColor("채팅 서버가 시작되었습니다.", ConsoleColor::Green);
        ConsoleHelper::Out("주소: " + g_server->GetServerAddress() + ":" + std::to_string(g_server->GetServerPort()));
        ConsoleHelper::Out("Ctrl+C를 눌러 서버를 종료할 수 있습니다.");

        // 메인 스레드 유지
        while (g_running && g_server->IsRunning()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // 주기적으로 서버 상태 정보 출력 (예: 10초마다)
            static int counter = 0;
            if (++counter % 100 == 0) {  // 100 * 100ms = 10초
                ConsoleHelper::Out("연결된 클라이언트 수: " + std::to_string(g_server->GetConnectionCount()));
                ConsoleHelper::Out(g_server->GetServerStats());
            }
        }

        // 서버 종료 (시그널 핸들러에서 이미 호출될 수 있지만 안전장치)
        if (g_server && g_server->IsRunning()) {
            ConsoleHelper::OutWithColor("메인에서 서버를 종료합니다...", ConsoleColor::Yellow);
            try {
                g_server->Stop();
                ConsoleHelper::OutWithColor("메인에서 서버 종료 완료", ConsoleColor::Green);
            }
            catch (const std::runtime_error& e) {
                ConsoleHelper::OutWithColor("메인에서 서버 종료 중 runtime_error: " + std::string(e.what()), ConsoleColor::Red);
            }
            catch (const std::exception& e) {
                ConsoleHelper::OutWithColor("메인에서 서버 종료 중 예외 발생: " + std::string(e.what()), ConsoleColor::Red);
            }
            catch (...) {
                ConsoleHelper::OutWithColor("메인에서 서버 종료 중 알 수 없는 예외 발생", ConsoleColor::Red);
            }
        }
        
        // 정리 시간을 줌
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        
        // 서버 객체 안전하게 해제
        try {
            ConsoleHelper::OutWithColor("서버 객체 해제 시작...", ConsoleColor::Yellow);
            g_server.reset();
            ConsoleHelper::OutWithColor("서버 객체 해제 완료", ConsoleColor::Green);
        }
        catch (const std::runtime_error& e) {
            ConsoleHelper::OutWithColor("서버 객체 해제 중 runtime_error: " + std::string(e.what()), ConsoleColor::Red);
        }
        catch (const std::exception& e) {
            ConsoleHelper::OutWithColor("서버 객체 해제 중 예외: " + std::string(e.what()), ConsoleColor::Red);
        }
        catch (...) {
            ConsoleHelper::OutWithColor("서버 객체 해제 중 알 수 없는 예외", ConsoleColor::Red);
        }
        
        ConsoleHelper::OutWithColor("서버가 종료되었습니다.", ConsoleColor::Green);

        // 콘솔 핸들러 해제
        SetConsoleCtrlHandler(ConsoleHandler, FALSE);

    }
    catch (const std::exception& e) {
        ConsoleHelper::OutWithColor("예외 발생: " + std::string(e.what()), ConsoleColor::Red);
        
        // 서버 객체 강제 해제
        try {
            g_server.reset();
        }
        catch (...) {
            // 무시
        }
        
        return 1;
    }
    catch (...) {
        ConsoleHelper::OutWithColor("알 수 없는 예외 발생", ConsoleColor::Red);
        
        // 서버 객체 강제 해제
        try {
            g_server.reset();
        }
        catch (...) {
            // 무시
        }
        
        return 1;
    }

    return 0;
}
