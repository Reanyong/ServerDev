#pragma once

#include <pqxx/pqxx>
#include <string>
#include <memory>
#include <mutex>
#include <functional>

class DatabaseManager {
private:
    // singleton instance
    static std::unique_ptr<DatabaseManager> instance_;
    static std::mutex instance_mutex_;

    // DB Connection information
    std::string connection_string_;

    // connection pool
    std::unique_ptr<pqxx::connection> connection_;
    std::mutex conn_mutex_;

    // singleton constructor
    DatabaseManager(const std::string& connection_string);

public:
    // singleton instance accessor
    static DatabaseManager& getInstance();

    // initialize function
    static void initatlize(const std::string& connection_string);

    // destructor
    ~DatabaseManager();

    // connection test
    bool testConnection();

    // transaction func
    template<typename Func>
    bool executeTransaction(Func&& func);

    // query func
    pqxx::result executeQuery(const std::string& query);

    // db connection getter
    pqxx::connection& getConnection();
};

// transaction template func
template<typename Func>
bool DatabaseManager::executeTransaction(Func&& func) {
    try {
        std::lock_guard<std::mutex> lock(conn_mutex_);

        if (!conn_->is_open()) {
            conn_ = std::make_unique<pqxx::connection>(connection_string_);
        }

        pqxx::work txn(*connection_);

        func(txn);

        txn.commit();

        return true;
    }
    catch (const std::exception& e) {
        ConsoleErr(utf8_to_wstring(std::string("DB 트랜잭션 실패: ") + e.what()));
        return false;
    }
}
