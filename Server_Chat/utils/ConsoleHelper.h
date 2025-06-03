// ConsoleHelper.h
#pragma once

#include <string>
#include <mutex>
#include <Windows.h>

enum class ConsoleColor {
    Black = 0,
    DarkBlue = 1,
    DarkGreen = 2,
    DarkCyan = 3,
    DarkRed = 4,
    DarkMagenta = 5,
    DarkYellow = 6,
    Gray = 7,
    DarkGray = 8,
    Blue = 9,
    Green = 10,
    Cyan = 11,
    Red = 12,
    Magenta = 13,
    Yellow = 14,
    White = 15
};

class ConsoleHelper {
public:
    // 콘솔 초기화 (UTF-8 설정 등)
    static bool Initialize();

    // 일반 출력 (스레드 안전)
    static void Out(const std::wstring& message);
    static void Out(const std::string& message);

    // 오류 출력 (스레드 안전)
    static void Error(const std::wstring& message);
    static void Error(const std::string& message);

    // 색상 지정 출력
    static void OutWithColor(const std::wstring& message, ConsoleColor textColor);
    static void OutWithColor(const std::string& message, ConsoleColor textColor);

    // 특수 포맷 출력 (현재 스레드 ID 등 포함)
    static void ThreadOut(const std::wstring& message);
    static void ThreadOut(const std::string& message);

    // 디버그 모드 전용 출력 (릴리즈 빌드에서는 무시됨)
    static void Debug(const std::wstring& message);
    static void Debug(const std::string& message);

private:
    static std::mutex console_mutex_;
    static HANDLE stdout_handle_;
    static HANDLE stderr_handle_;

    // 내부 구현용 핵심 출력 함수
    static void WriteToConsole(HANDLE handle, const std::wstring& message, bool newline = true);
    static void SetConsoleTextColor(HANDLE handle, ConsoleColor color);
    static void ResetConsoleTextColor(HANDLE handle);
};

// 간편 매크로 정의
#ifdef _DEBUG
#define DEBUG_OUT(msg) ConsoleHelper::Debug(msg)
#else
#define DEBUG_OUT(msg) do {} while(0)
#endif
