// DatabaseManager.cpp
#include "DatabaseManager.h"
#include "../utils/StringUtils.h"
#include "../utils/ConsoleHelper.h"
#include <iostream>
#include <string>
#include <Windows.h>

// 싱글톤 인스턴스 초기화
std::unique_ptr<DatabaseManager> DatabaseManager::instance_ = nullptr;
std::mutex DatabaseManager::instance_mutex_;

// 생성자
DatabaseManager::DatabaseManager(const std::string& connection_string)
    : connection_string_(connection_string) {
    try {
        // DB 연결 시도
        conn_ = std::make_unique<pqxx::connection>(connection_string);
        ConsoleHelper::Out(L"[DB] PostgreSQL 연결 성공");
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error(StringUtils::Utf8ToWString(std::string("PostgreSQL 연결 실패: ") + e.what()));
        throw; // 초기화 실패 시 예외 전파
    }
}

// 소멸자
DatabaseManager::~DatabaseManager() {
    try {
        if (conn_ && conn_->is_open()) {
            // 디버그 모드 예외 발생 가능한 부분
#ifdef NDEBUG
            conn_->close();
#endif
            ConsoleHelper::Out(L"[DB] PostgreSQL 연결 종료");
        }
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error(StringUtils::Utf8ToWString(std::string("PostgreSQL 연결 종료 실패: ") + e.what()));
    }
}

// 싱글톤 인스턴스 접근자
DatabaseManager& DatabaseManager::getInstance() {
    if (!instance_) {
        throw std::runtime_error("DatabaseManager가 초기화되지 않았습니다. initialize()를 먼저 호출하세요.");
    }
    return *instance_;
}

// 초기화 함수
void DatabaseManager::initialize(const std::string& connection_string) {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    if (!instance_) {
        instance_ = std::unique_ptr<DatabaseManager>(new DatabaseManager(connection_string));
    }
}

// 연결 테스트
bool DatabaseManager::testConnection() {
    try {
        std::lock_guard<std::mutex> lock(conn_mutex_);

        if (!conn_->is_open()) {
            conn_ = std::make_unique<pqxx::connection>(connection_string_);
        }

        // 간단한 쿼리로 연결 테스트
        pqxx::work txn(*conn_);
        pqxx::result r = txn.exec("SELECT 1 AS test");
        txn.commit();

        return r.size() > 0 && r[0][0].as<int>() == 1;
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error(StringUtils::Utf8ToWString(std::string("DB 연결 테스트 실패: ") + e.what()));
        return false;
    }
}

// 쿼리 실행
pqxx::result DatabaseManager::executeQuery(const std::string& query) {
    pqxx::result result;
    try {
        std::lock_guard<std::mutex> lock(conn_mutex_);

        if (!conn_->is_open()) {
            conn_ = std::make_unique<pqxx::connection>(connection_string_);
        }

        pqxx::work txn(*conn_);
        result = txn.exec(query);
        txn.commit();
    }
    catch (const std::exception& e) {
        ConsoleHelper::Error(StringUtils::Utf8ToWString(std::string("쿼리 실행 실패: ") + e.what()));
        throw; // 쿼리 실패 시 예외 전파
    }
    return result;
}

// DB 연결 객체 가져오기
pqxx::connection& DatabaseManager::getConnection() {
    std::lock_guard<std::mutex> lock(conn_mutex_);
    if (!conn_->is_open()) {
        conn_ = std::make_unique<pqxx::connection>(connection_string_);
    }
    return *conn_;
}
