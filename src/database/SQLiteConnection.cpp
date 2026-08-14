#include "database/SQLiteConnection.h"

namespace dbterm {

SQLiteConnection::SQLiteConnection() = default;

SQLiteConnection::~SQLiteConnection() {
    disconnect();
}

bool SQLiteConnection::connect(const ConnectionConfig& config, std::string& errorOut) {
    disconnect();
    config_ = config;

    std::string dbPath = config_.host.empty() ? config_.database : config_.host;
    if (dbPath.empty()) {
        dbPath = ":memory:";
    }

    if (sqlite3_open(dbPath.c_str(), &db_) != SQLITE_OK) {
        errorOut = db_ ? sqlite3_errmsg(db_) : "Failed to open SQLite database file.";
        if (db_) {
            sqlite3_close(db_);
            db_ = nullptr;
        }
        connected_ = false;
        return false;
    }

    connected_ = true;
    return true;
}

void SQLiteConnection::disconnect() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
    connected_ = false;
}

bool SQLiteConnection::isConnected() const {
    return connected_ && (db_ != nullptr);
}

bool SQLiteConnection::ping(std::string& errorOut) {
    if (!isConnected()) {
        errorOut = "Not connected to SQLite database.";
        return false;
    }
    return true;
}

bool SQLiteConnection::selectDatabase(const std::string& /*dbName*/, std::string& /*errorOut*/) {
    return true;
}

QueryResult SQLiteConnection::executeQuery(const std::string& sql) {
    QueryResult qr;
    if (!isConnected()) {
        qr.success = false;
        qr.errorMessage = "Not connected to an SQLite database.";
        return qr;
    }

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        qr.success = false;
        qr.errorMessage = sqlite3_errmsg(db_);
        return qr;
    }

    int colCount = sqlite3_column_count(stmt);
    for (int i = 0; i < colCount; ++i) {
        const char* name = sqlite3_column_name(stmt, i);
        qr.columns.push_back(name ? name : "");
    }

    int rc;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        std::vector<std::string> row;
        row.reserve(colCount);
        for (int i = 0; i < colCount; ++i) {
            const unsigned char* text = sqlite3_column_text(stmt, i);
            if (text) {
                row.push_back(reinterpret_cast<const char*>(text));
            } else {
                row.push_back("NULL");
            }
        }
        qr.rows.push_back(std::move(row));
    }

    if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
        qr.success = false;
        qr.errorMessage = sqlite3_errmsg(db_);
    } else {
        qr.success = true;
    }

    sqlite3_finalize(stmt);
    return qr;
}

std::vector<std::string> SQLiteConnection::getDatabases(std::string& /*errorOut*/) {
    return {"main"};
}

std::vector<std::string> SQLiteConnection::getTables(const std::string& /*dbName*/, std::string& errorOut) {
    std::vector<std::string> tables;
    QueryResult qr = executeQuery("SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%';");
    if (!qr.success) {
        errorOut = qr.errorMessage;
        return tables;
    }

    for (const auto& row : qr.rows) {
        if (!row.empty()) {
            tables.push_back(row[0]);
        }
    }
    return tables;
}

QueryResult SQLiteConnection::getTableStructure(const std::string& /*dbName*/, const std::string& tableName, std::string& /*errorOut*/) {
    std::string sql = "PRAGMA table_info(\"" + tableName + "\");";
    return executeQuery(sql);
}

} // namespace dbterm
