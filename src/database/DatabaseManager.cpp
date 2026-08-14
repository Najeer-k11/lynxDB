#include "database/DatabaseManager.h"
#include "database/MySQLConnection.h"
#include "database/SQLiteConnection.h"

namespace dbterm {

bool DatabaseManager::connect(const ConnectionConfig& config, std::string& errorOut) {
    disconnect();

    if (config.type == DatabaseType::MYSQL) {
        auto conn = std::make_unique<MySQLConnection>();
        if (conn->connect(config, errorOut)) {
            connection_ = std::move(conn);
            return true;
        }
        return false;
    } else if (config.type == DatabaseType::SQLITE) {
        auto conn = std::make_unique<SQLiteConnection>();
        if (conn->connect(config, errorOut)) {
            connection_ = std::move(conn);
            return true;
        }
        return false;
    }

    errorOut = "Unsupported database type.";
    return false;
}

void DatabaseManager::disconnect() {
    if (connection_) {
        connection_->disconnect();
        connection_.reset();
    }
}

bool DatabaseManager::isConnected() const {
    return connection_ != nullptr && connection_->isConnected();
}

DatabaseConnection* DatabaseManager::activeConnection() {
    return connection_.get();
}

const ConnectionConfig* DatabaseManager::activeConfig() const {
    if (connection_) {
        return &connection_->getConfig();
    }
    return nullptr;
}

} // namespace dbterm
