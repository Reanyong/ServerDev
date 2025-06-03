// ConsoleHelper.cpp
#include "ConsoleHelper.h"
#include "StringUtils.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <locale>

// 정적 멤버 초기화
std::mutex ConsoleHelper::console_mutex_;
HANDLE ConsoleHelper::stdout_handle_ = GetStdHandle(STD_OUTPUT_HANDLE);
HANDLE ConsoleHelper::stderr_handle_ = GetStdHandle(STD_ERROR_HANDLE);

bool ConsoleHelper::Initialize() {
    try {
        // 콘솔 코드 페이지 설정 (강화)
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);

        // 추가 콘솔 모드 설정
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
        
        if (hOut != INVALID_HANDLE_VALUE) {
            DWORD dwMode = 0;
            GetConsoleMode(hOut, &dwMode);
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }

        if (hIn != INVALID_HANDLE_VALUE) {
            DWORD dwMode = 0;
            GetConsoleMode(hIn, &dwMode);
            dwMode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
            SetConsoleMode(hIn, dwMode);
        }

        // 로케일 설정 강화
        setlocale(LC_ALL, "ko_KR.UTF-8");
        setlocale(LC_CTYPE, "UTF-8");
        
        // C++ locale 설정
        try {
            std::locale::global(std::locale("ko_KR.UTF-8"));
        }
        catch (...) {
            try {
                std::locale::global(std::locale("Korean_Korea.UTF-8"));
            }
            catch (...) {
                // 기본 로케일 사용
                std::locale::global(std::locale(""));
            }
        }

        // 핸들 유효성 검사
        if (stdout_handle_ == INVALID_HANDLE_VALUE || stderr_handle_ == INVALID_HANDLE_VALUE) {
            std::cerr << "Console handles initialization failed" << std::endl;
            return false;
        }

        // 콘솔 폰트 설정 (UTF-8 지원)
        CONSOLE_FONT_INFOEX fontInfo = { 0 };
        fontInfo.cbSize = sizeof(fontInfo);
        if (GetCurrentConsoleFontEx(stdout_handle_, FALSE, &fontInfo)) {
            wcscpy_s(fontInfo.FaceName, L"Consolas");
            fontInfo.FontWeight = 400;
            SetCurrentConsoleFontEx(stdout_handle_, FALSE, &fontInfo);
        }

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Console initialization failed: " << e.what() << std::endl;
        return false;
    }
}

void ConsoleHelper::Out(const std::wstring& message) {
    std::lock_guard<std::mutex> lock(console_mutex_);

    try {
        WriteToConsole(stdout_handle_, message, true);
    }
    catch (const std::exception& e) {
        // 출력 실패 시 대체 방법 시도
        std::wcerr << L"출력 오류: " << StringUtils::Utf8ToWString(e.what()) << std::endl;
        std::wcout << message << std::endl;
    }
}

void ConsoleHelper::Out(const std::string& message) {
    Out(StringUtils::Utf8ToWString(message));
}

void ConsoleHelper::Error(const std::wstring& message) {
    std::lock_guard<std::mutex> lock(console_mutex_);

    try {
        // 에러 출력은 빨간색으로 표시
        SetConsoleTextColor(stderr_handle_, ConsoleColor::Red);
        WriteToConsole(stderr_handle_, message, true);
        ResetConsoleTextColor(stderr_handle_);
    }
    catch (const std::exception& e) {
        // 출력 실패 시 대체 방법 시도
        std::wcerr << L"오류 출력 실패: " << StringUtils::Utf8ToWString(e.what()) << std::endl;
        std::wcerr << message << std::endl;
    }
}

void ConsoleHelper::Error(const std::string& message) {
    Error(StringUtils::Utf8ToWString(message));
}

void ConsoleHelper::OutWithColor(const std::wstring& message, ConsoleColor textColor) {
    std::lock_guard<std::mutex> lock(console_mutex_);

    try {
        SetConsoleTextColor(stdout_handle_, textColor);
        WriteToConsole(stdout_handle_, message, true);
        ResetConsoleTextColor(stdout_handle_);
    }
    catch (const std::exception& e) {
        // 출력 실패 시 대체 방법 시도
        std::wcout << message << std::endl;
    }
}

void ConsoleHelper::OutWithColor(const std::string& message, ConsoleColor textColor) {
    OutWithColor(StringUtils::Utf8ToWString(message), textColor);
}

void ConsoleHelper::ThreadOut(const std::wstring& message) {
    std::wstringstream ss;
    ss << L"[Thread ID: " << std::this_thread::get_id() << L"] " << message;
    Out(ss.str());
}

void ConsoleHelper::ThreadOut(const std::string& message) {
    ThreadOut(StringUtils::Utf8ToWString(message));
}

void ConsoleHelper::Debug(const std::wstring& message) {
#ifdef _DEBUG
    std::wstringstream ss;
    ss << L"[DEBUG] " << message;
    OutWithColor(ss.str(), ConsoleColor::DarkCyan);
#endif
}

void ConsoleHelper::Debug(const std::string& message) {
    Debug(StringUtils::Utf8ToWString(message));
}

void ConsoleHelper::WriteToConsole(HANDLE handle, const std::wstring& message, bool newline) {
    if (handle == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Invalid console handle");
    }

    DWORD written = 0;

    if (!WriteConsoleW(handle, message.c_str(), (DWORD)message.length(), &written, nullptr)) {
        DWORD error = GetLastError();
        throw std::runtime_error("Failed to write to console: " + std::to_string(error));
    }

    if (newline) {
        WriteConsoleW(handle, L"\r\n", 2, &written, nullptr);
    }
}

void ConsoleHelper::SetConsoleTextColor(HANDLE handle, ConsoleColor color) {
    if (handle == INVALID_HANDLE_VALUE) {
        return;
    }

    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(handle, &info)) {
        return;
    }

    WORD attributes = (info.wAttributes & 0xFFF0) | static_cast<WORD>(color);
    SetConsoleTextAttribute(handle, attributes);
}

void ConsoleHelper::ResetConsoleTextColor(HANDLE handle) {
    if (handle == INVALID_HANDLE_VALUE) {
        return;
    }

    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(handle, &info)) {
        return;
    }

    // 기본 텍스트 색상으로 복원 (흰색)
    SetConsoleTextAttribute(handle, (info.wAttributes & 0xFFF0) | static_cast<WORD>(ConsoleColor::Gray));
}
