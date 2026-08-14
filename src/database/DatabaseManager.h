#ifndef DBTERM_DATABASEMANAGER_H
#define DBTERM_DATABASEMANAGER_H

#include "database/DatabaseConnection.h"
#include "models/ConnectionConfig.h"
#include <memory>
#include <string>

namespace dbterm {

class DatabaseManager {
public:
    DatabaseManager() = default;
    ~DatabaseManager() = default;

    bool connect(const ConnectionConfig& config, std::string& errorOut);
    void disconnect();
    bool isConnected() const;
    DatabaseConnection* activeConnection();
    const ConnectionConfig* activeConfig() const;

private:
    std::unique_ptr<DatabaseConnection> connection_;
};

} // namespace dbterm

#endif // DBTERM_DATABASEMANAGER_H
