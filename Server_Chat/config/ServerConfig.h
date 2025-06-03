// ServerConfig.h
#pragma once

#include <string>
#include <optional>
#include <vector>
#include <unordered_map>

class ServerConfig {
public:
    // 기본 생성자
    ServerConfig();

    // 파일에서 설정 로드
    bool loadFromFile(const std::string& filename);

    // 설정 값 가져오기
    std::string getServerAddress() const { return server_address_; }
    uint16_t getServerPort() const { return server_port_; }
    int getThreadCount() const { return thread_count_; }
    const std::string& getLogLevel() const { return log_level_; }
    bool isDebugMode() const { return debug_mode_; }

    // DB 관련 설정
    const std::string& getDbHost() const { return db_host_; }
    uint16_t getDbPort() const { return db_port_; }
    const std::string& getDbName() const { return db_name_; }
    const std::string& getDbUser() const { return db_user_; }
    const std::string& getDbPassword() const { return db_password_; }

    // 설정 값 변경
    void setServerAddress(const std::string& address) { server_address_ = address; }
    void setServerPort(uint16_t port) { server_port_ = port; }
    void setThreadCount(int count) { thread_count_ = count; }
    void setLogLevel(const std::string& level) { log_level_ = level; }
    void setDebugMode(bool debug) { debug_mode_ = debug; }

    // DB 설정 변경
    void setDbHost(const std::string& host) { db_host_ = host; }
    void setDbPort(uint16_t port) { db_port_ = port; }
    void setDbName(const std::string& name) { db_name_ = name; }
    void setDbUser(const std::string& user) { db_user_ = user; }
    void setDbPassword(const std::string& password) { db_password_ = password; }

    // DB 연결 문자열 생성
    std::string getDbConnectionString() const;

    // 전체 설정 정보 문자열 반환 (디버깅용)
    std::string toString() const;

    // 인증 타임아웃 설정
    unsigned int getAuthTimeoutSeconds() const { return auth_timeout_seconds_; }
    void setAuthTimeoutSeconds(unsigned int timeout) { auth_timeout_seconds_ = timeout; }

    // 핑/퐁 시간 간격
    unsigned int getPingIntervalSeconds() const { return ping_interval_seconds_; }
    void setPingIntervalSeconds(unsigned int interval) { ping_interval_seconds_ = interval; }

private:
    // 서버 설정
    std::string server_address_ = "0.0.0.0";  // 모든 인터페이스에서 수신
    uint16_t server_port_ = 8080;            // 기본 포트
    int thread_count_ = 0;                    // 0이면 하드웨어 스레드 수 사용
    std::string log_level_ = "info";          // 로그 레벨
    bool debug_mode_ = false;                 // 디버그 모드 여부

    // DB 설정
    std::string db_host_ = "localhost";
    uint16_t db_port_ = 5432;
    std::string db_name_ = "fps_game_db";
    std::string db_user_ = "fps_user";
    std::string db_password_ = "3567";

    // 타임아웃 설정
    unsigned int auth_timeout_seconds_ = 60;   // 인증 타임아웃
    unsigned int ping_interval_seconds_ = 30;  // 핑 간격

    // 설정 파일에서 키-값 파싱
    std::unordered_map<std::string, std::string> parseConfigFile(const std::string& filename);
};
