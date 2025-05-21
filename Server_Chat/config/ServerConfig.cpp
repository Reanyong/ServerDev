// ServerConfig.cpp
#include "ServerConfig.h"
#include <fstream>
#include <sstream>
#include <iostream>

ServerConfig::ServerConfig() {
    // 기본값 설정
    server_address_ = "0.0.0.0";      // 모든 인터페이스에서 수신
    server_port_ = 8080;              // 기본 포트
    thread_count_ = 0;                // 0이면 하드웨어 스레드 수 사용
    log_level_ = "info";              // 로그 레벨
    debug_mode_ = false;              // 디버그 모드 여부

    // DB 설정
    db_host_ = "localhost";
    db_port_ = 5432;
    db_name_ = "fps_game_db";
    db_user_ = "fps_user";
    db_password_ = "3567";

    // 타임아웃 설정
    auth_timeout_seconds_ = 60;       // 인증 타임아웃
    ping_interval_seconds_ = 30;      // 핑 간격
}

bool ServerConfig::loadFromFile(const std::string& filename) {
    try {
        auto settings = parseConfigFile(filename);

        // 서버 설정 적용
        if (settings.count("server_address")) server_address_ = settings["server_address"];
        if (settings.count("server_port")) server_port_ = static_cast<uint16_t>(std::stoi(settings["server_port"]));
        if (settings.count("thread_count")) thread_count_ = std::stoi(settings["thread_count"]);
        if (settings.count("log_level")) log_level_ = settings["log_level"];
        if (settings.count("debug_mode")) debug_mode_ = settings["debug_mode"] == "true" || settings["debug_mode"] == "1";

        // DB 설정 적용
        if (settings.count("db_host")) db_host_ = settings["db_host"];
        if (settings.count("db_port")) db_port_ = static_cast<uint16_t>(std::stoi(settings["db_port"]));
        if (settings.count("db_name")) db_name_ = settings["db_name"];
        if (settings.count("db_user")) db_user_ = settings["db_user"];
        if (settings.count("db_password")) db_password_ = settings["db_password"];

        // 타임아웃 설정 적용
        if (settings.count("auth_timeout_seconds")) auth_timeout_seconds_ = std::stoi(settings["auth_timeout_seconds"]);
        if (settings.count("ping_interval_seconds")) ping_interval_seconds_ = std::stoi(settings["ping_interval_seconds"]);

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "설정 파일 로드 실패: " << e.what() << std::endl;
        return false;
    }
}

std::string ServerConfig::getDbConnectionString() const {
    std::stringstream ss;
    ss << "host=" << db_host_
        << " port=" << db_port_
        << " dbname=" << db_name_
        << " user=" << db_user_
        << " password=" << db_password_;
    return ss.str();
}

std::string ServerConfig::toString() const {
    std::stringstream ss;
    ss << "=== 서버 설정 ===" << std::endl
        << "서버 주소: " << server_address_ << std::endl
        << "서버 포트: " << server_port_ << std::endl
        << "스레드 수: " << (thread_count_ > 0 ? std::to_string(thread_count_) : "자동(하드웨어 스레드 수)") << std::endl
        << "로그 레벨: " << log_level_ << std::endl
        << "디버그 모드: " << (debug_mode_ ? "활성화" : "비활성화") << std::endl
        << std::endl
        << "=== DB 설정 ===" << std::endl
        << "DB 호스트: " << db_host_ << std::endl
        << "DB 포트: " << db_port_ << std::endl
        << "DB 이름: " << db_name_ << std::endl
        << "DB 사용자: " << db_user_ << std::endl
        << "DB 비밀번호: " << (db_password_.empty() ? "없음" : "****") << std::endl
        << std::endl
        << "=== 타임아웃 설정 ===" << std::endl
        << "인증 타임아웃: " << auth_timeout_seconds_ << "초" << std::endl
        << "핑 간격: " << ping_interval_seconds_ << "초" << std::endl;
    return ss.str();
}

std::unordered_map<std::string, std::string> ServerConfig::parseConfigFile(const std::string& filename) {
    std::unordered_map<std::string, std::string> settings;
    std::ifstream file(filename);

    if (!file.is_open()) {
        throw std::runtime_error("설정 파일을 열 수 없습니다: " + filename);
    }

    std::string line;
    std::string currentSection;

    while (std::getline(file, line)) {
        // 공백 및 주석 제거
        size_t commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        // 앞뒤 공백 제거
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty()) {
            continue;
        }

        // 섹션 헤더 확인 [Section]
        if (line.front() == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.size() - 2);
            continue;
        }

        // 키=값 쌍 파싱
        size_t equalPos = line.find('=');
        if (equalPos != std::string::npos) {
            std::string key = line.substr(0, equalPos);
            std::string value = line.substr(equalPos + 1);

            // 앞뒤 공백 제거
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            // 섹션이 있으면 키에 섹션 접두어 추가
            if (!currentSection.empty()) {
                key = currentSection + "." + key;
            }

            settings[key] = value;
        }
    }

    return settings;
}
