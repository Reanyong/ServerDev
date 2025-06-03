// StringUtils.h
#pragma once

#include <string>
#include <vector>

class StringUtils {
public:
    // UTF-8 문자열을 와이드 문자열로 변환
    static std::wstring Utf8ToWString(const std::string& str);

    // 와이드 문자열을 UTF-8 문자열로 변환
    static std::string WStringToUtf8(const std::wstring& wstr);

    // 문자열 앞뒤 공백 제거
    static std::string Trim(const std::string& str);

    // 문자열을 지정된 구분자로 분할
    static std::vector<std::string> Split(const std::string& str, const std::string& delimiter);

    // 형식화된 문자열 생성 (printf 스타일)
    template<typename... Args>
    static std::string Format(const char* format, Args... args);

    // 형식화된 와이드 문자열 생성
    template<typename... Args>
    static std::wstring FormatW(const wchar_t* format, Args... args);
};

// StringUtils.cpp의 구현 예시 (템플릿 함수는 헤더에 정의 필요)
template<typename... Args>
std::string StringUtils::Format(const char* format, Args... args) {
    // _vscprintf는 널 종료 문자를 제외한 필요한 버퍼 크기를 계산
    int size = _vscprintf(format, args...) + 1;
    std::vector<char> buffer(size);
    vsprintf_s(buffer.data(), size, format, args...);
    return std::string(buffer.data(), buffer.data() + size - 1);
}

template<typename... Args>
std::wstring StringUtils::FormatW(const wchar_t* format, Args... args) {
    // _vscwprintf는 널 종료 문자를 제외한 필요한 버퍼 크기를 계산
    int size = _vscwprintf(format, args...) + 1;
    std::vector<wchar_t> buffer(size);
    vswprintf_s(buffer.data(), size, format, args...);
    return std::wstring(buffer.data(), buffer.data() + size - 1);
}
