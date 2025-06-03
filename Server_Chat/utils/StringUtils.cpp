// StringUtils.cpp
#include "StringUtils.h"
#include <Windows.h>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <stdexcept>

std::wstring StringUtils::Utf8ToWString(const std::string& str) {
    if (str.empty()) return std::wstring();

    try {
        // 필요한 버퍼 크기 계산
        int size_need = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
        if (size_need <= 0) {
            // 변환 실패 시 오류 내용을 직접 확인
            DWORD error = GetLastError();
            return L"<변환 오류: " + std::to_wstring(error) + L">";
        }

        // 버퍼 할당 및 변환
        std::wstring result(size_need, 0);
        if (MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &result[0], size_need) <= 0) {
            DWORD error = GetLastError();
            return L"<변환 오류: " + std::to_wstring(error) + L">";
        }

        return result;
    }
    catch (const std::exception& e) {
        // 예외 발생 시 안전한 문자열 반환
        std::string what_str = e.what(); // 문자열로 변환
        std::wstring what_wstr;

        // 간단히 문자 단위로 변환 (ASCII 문자만 정상 처리됨)
        for (char c : what_str) {
            what_wstr.push_back(static_cast<wchar_t>(c));
        }

        return L"<예외 발생: " + what_wstr + L">";
    }
}

std::string StringUtils::WStringToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();

    try {
        // 필요한 버퍼 크기 계산
        int size_need = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
        if (size_need <= 0) {
            DWORD error = GetLastError();
            return "<변환 오류: " + std::to_string(error) + ">";
        }

        // 버퍼 할당 및 변환
        std::string result(size_need, 0);
        if (WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &result[0], size_need, nullptr, nullptr) <= 0) {
            DWORD error = GetLastError();
            return "<변환 오류: " + std::to_string(error) + ">";
        }

        return result;
    }
    catch (const std::exception& e) {
        return "<예외 발생: " + std::string(e.what()) + ">";
    }
}

std::string StringUtils::Trim(const std::string& str) {
    if (str.empty()) return str;

    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return "";

    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::vector<std::string> StringUtils::Split(const std::string& str, const std::string& delimiter) {
    std::vector<std::string> tokens;
    if (str.empty()) return tokens;
    if (delimiter.empty()) {
        tokens.push_back(str);
        return tokens;
    }

    size_t start = 0;
    size_t end = 0;
    while ((end = str.find(delimiter, start)) != std::string::npos) {
        tokens.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
    }
    // 마지막 토큰 추가
    tokens.push_back(str.substr(start));

    return tokens;
}
