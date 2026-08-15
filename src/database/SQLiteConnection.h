#ifndef DBTERM_SQLITECONNECTION_H
#define DBTERM_SQLITECONNECTION_H

#include "database/DatabaseConnection.h"
#include <sqlite3.h>

namespace dbterm {

class SQLiteConnection : public DatabaseConnection {
public:
    SQLiteConnection();
    ~SQLiteConnection() override;

    bool connect(const ConnectionConfig& config, std::string& errorOut) override;
    void disconnect() override;
    bool isConnected() const override;
    bool ping(std::string& errorOut) override;
    const ConnectionConfig& getConfig() const override { return config_; }

    QueryResult executeQuery(const std::string& sql) override;
    std::vector<std::string> getDatabases(std::string& errorOut) override;
    std::vector<std::string> getTables(const std::string& dbName, std::string& errorOut) override;
    QueryResult getTableStructure(const std::string& dbName, const std::string& tableName, std::string& errorOut) override;
    std::vector<ForeignKeyInfo> getForeignKeys(const std::string& dbName, const std::string& tableName, std::string& errorOut) override;
    bool selectDatabase(const std::string& dbName, std::string& errorOut) override;

private:
    sqlite3* db_{nullptr};
    bool connected_{false};
    ConnectionConfig config_;
};

} // namespace dbterm

#endif // DBTERM_SQLITECONNECTION_H
