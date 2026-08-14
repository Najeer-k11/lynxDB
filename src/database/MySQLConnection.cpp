#include "database/MySQLConnection.h"

namespace dbterm {

MySQLConnection::MySQLConnection() = default;

MySQLConnection::~MySQLConnection() {
    disconnect();
}

bool MySQLConnection::connect(const ConnectionConfig& config, std::string& errorOut) {
    disconnect();
    config_ = config;

    mysql_ = mysql_init(nullptr);
    if (!mysql_) {
        errorOut = "Failed to initialize MySQL handle (out of memory).";
        return false;
    }

    unsigned int timeout = 5;
    mysql_options(mysql_, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

    const char* host = config_.host.empty() ? "127.0.0.1" : config_.host.c_str();
    const char* user = config_.user.empty() ? nullptr : config_.user.c_str();
    const char* pass = config_.password.empty() ? nullptr : config_.password.c_str();
    const char* db   = config_.database.empty() ? nullptr : config_.database.c_str();
    unsigned int port = static_cast<unsigned int>(config_.port > 0 ? config_.port : 3306);

    MYSQL* res = mysql_real_connect(mysql_, host, user, pass, db, port, nullptr, 0);
    if (!res) {
        errorOut = mysql_error(mysql_);
        if (errorOut.empty()) {
            errorOut = "Failed to connect to MySQL server at " + config_.host + ":" + std::to_string(config_.port);
        }
        mysql_close(mysql_);
        mysql_ = nullptr;
        connected_ = false;
        return false;
    }

    connected_ = true;
    return true;
}

void MySQLConnection::disconnect() {
    if (mysql_) {
        mysql_close(mysql_);
        mysql_ = nullptr;
    }
    connected_ = false;
}

bool MySQLConnection::isConnected() const {
    return connected_ && (mysql_ != nullptr);
}

bool MySQLConnection::ping(std::string& errorOut) {
    if (!isConnected()) {
        errorOut = "Not connected.";
        return false;
    }

    if (mysql_ping(mysql_) != 0) {
        errorOut = mysql_error(mysql_);
        connected_ = false;
        return false;
    }

    return true;
}

bool MySQLConnection::selectDatabase(const std::string& dbName, std::string& errorOut) {
    if (!isConnected()) {
        errorOut = "Not connected.";
        return false;
    }

    if (mysql_select_db(mysql_, dbName.c_str()) != 0) {
        errorOut = mysql_error(mysql_);
        return false;
    }

    config_.database = dbName;
    return true;
}

QueryResult MySQLConnection::executeQuery(const std::string& sql) {
    QueryResult qr;
    if (!isConnected()) {
        qr.success = false;
        qr.errorMessage = "Not connected to a database server.";
        return qr;
    }

    if (mysql_query(mysql_, sql.c_str()) != 0) {
        qr.success = false;
        qr.errorMessage = mysql_error(mysql_);
        return qr;
    }

    MYSQL_RES* res = mysql_store_result(mysql_);
    if (!res) {
        if (mysql_field_count(mysql_) == 0) {
            qr.success = true;
            qr.affectedRows = mysql_affected_rows(mysql_);
            return qr;
        } else {
            qr.success = false;
            qr.errorMessage = mysql_error(mysql_);
            return qr;
        }
    }

    qr.success = true;
    unsigned int numFields = mysql_num_fields(res);
    MYSQL_FIELD* fields = mysql_fetch_fields(res);

    for (unsigned int i = 0; i < numFields; ++i) {
        qr.columns.push_back(fields[i].name ? fields[i].name : "");
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        unsigned long* lengths = mysql_fetch_lengths(res);
        std::vector<std::string> rowVec;
        rowVec.reserve(numFields);
        for (unsigned int i = 0; i < numFields; ++i) {
            if (row[i]) {
                rowVec.push_back(std::string(row[i], lengths[i]));
            } else {
                rowVec.push_back("NULL");
            }
        }
        qr.rows.push_back(std::move(rowVec));
    }

    mysql_free_result(res);
    return qr;
}

std::vector<std::string> MySQLConnection::getDatabases(std::string& errorOut) {
    std::vector<std::string> dbs;
    QueryResult qr = executeQuery("SHOW DATABASES;");
    if (!qr.success) {
        errorOut = qr.errorMessage;
        return dbs;
    }

    for (const auto& row : qr.rows) {
        if (!row.empty()) {
            dbs.push_back(row[0]);
        }
    }
    return dbs;
}

std::vector<std::string> MySQLConnection::getTables(const std::string& dbName, std::string& errorOut) {
    std::vector<std::string> tables;
    if (!dbName.empty()) {
        if (!selectDatabase(dbName, errorOut)) {
            return tables;
        }
    }

    QueryResult qr = executeQuery("SHOW TABLES;");
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

QueryResult MySQLConnection::getTableStructure(const std::string& dbName, const std::string& tableName, std::string& errorOut) {
    if (!dbName.empty()) {
        if (!selectDatabase(dbName, errorOut)) {
            QueryResult qr;
            qr.success = false;
            qr.errorMessage = errorOut;
            return qr;
        }
    }
    std::string sql = "DESCRIBE `" + tableName + "`;";
    return executeQuery(sql);
}

} // namespace dbterm
