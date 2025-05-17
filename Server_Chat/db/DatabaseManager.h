// DatabaseManager.h
#pragma once

#include <pqxx/pqxx>
#include <string>
#include <memory>
#include <mutex>
#include <functional>
#include <iostream>

class DatabaseManager {
private:
    // 싱글톤 인스턴스
    static std::unique_ptr<DatabaseManager> instance_;
    static std::mutex instance_mutex_;

    // DB 연결 정보
    std::string connection_string_;

    // 연결 객체
    std::unique_ptr<pqxx::connection> conn_;
    std::mutex conn_mutex_;

    // 싱글톤 패턴을 위한 생성자
    DatabaseManager(const std::string& connection_string);

public:
    // 싱글톤 인스턴스 접근자
    static DatabaseManager& getInstance();

    // 초기화 함수
    static void initialize(const std::string& connection_string);

    // 소멸자
    ~DatabaseManager();

    // DB 연결 테스트
    bool testConnection();

    // 트랜잭션 실행 헬퍼 함수
    template<typename Func>
    bool executeTransaction(Func&& func);

    // 쿼리 실행 헬퍼 함수
    pqxx::result executeQuery(const std::string& query);

    // DB 연결 객체 가져오기
    pqxx::connection& getConnection();
};

// 트랜잭션 실행 템플릿 함수 구현
template<typename Func>
bool DatabaseManager::executeTransaction(Func&& func) {
    try {
        std::lock_guard<std::mutex> lock(conn_mutex_);

        // 연결이 끊겼다면 재연결 시도
        if (!conn_->is_open()) {
            conn_ = std::make_unique<pqxx::connection>(connection_string_);
        }

        // 트랜잭션 생성
        pqxx::work txn(*conn_);

        // 함수 실행
        func(txn);

        // 커밋
        txn.commit();
        return true;
    }
    catch (const std::exception& e) {
        // 여기서 wstring 변환 함수를 직접 호출할 수 없으므로 
        // 일반 std::cerr을 사용
        std::cerr << "DB 트랜잭션 실패: " << e.what() << std::endl;
        return false;
    }
}
