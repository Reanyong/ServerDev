#include "DatabaseManager.h"
#include <iostream>

extern wstring utf8_to_wstring(const string& str);
extern void ConsoleOut(const wstring& message);
extern void ConsoleErr(const wstring& message);

std::unique_ptr<DatabaseManager> DatabaseManager::instance_ = nullptr;
std::mutex DatabaseManager::instance_mutex_;

DatabaseManager::DatabaseManager(const std::string& connection_string)
    : connection_string_(connection_string) {
    try {
        // DB 연결 시도
        conn_ = std::make_unique<pqxx::connection>(connection_string);
        ConsoleOut(L"[DB] PostgreSQL 연결 성공");
    }
    catch (const std::exception& e) {
        ConsoleErr(utf8_to_wstring(std::string("PostgreSQL 연결 실패: ") + e.what()));
        throw; // 초기화 실패 시 예외 전파
    }
}

DatabaseManager::~DatabaseManager() {
    try {
        if (conn_ && conn_->is_open()) {
            // 디버그 모드 예외 발생 가능한 부분
#ifdef NDEBUG
            conn_->close();
#endif
            ConsoleOut(L"[DB] PostgreSQL 연결 종료");
        }
    }
    catch (const std::exception& e) {
        ConsoleErr(utf8_to_wstring(std::string("PostgreSQL 연결 종료 실패: ") + e.what()));
    }
}

DatabaseManager& DatabaseManager::getInstance() {
    if (!instance_) {
        throw std::runtime_error("DatabaseManager가 초기화되지 않았습니다. initialize()를 먼저 호출하세요.");
    }
    return *instance_;
}
