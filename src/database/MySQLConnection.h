#ifndef DBTERM_MYSQLCONNECTION_H
#define DBTERM_MYSQLCONNECTION_H

#include "database/DatabaseConnection.h"
#include <mysql/mysql.h>

namespace dbterm {

class MySQLConnection : public DatabaseConnection {
public:
    MySQLConnection();
    ~MySQLConnection() override;

    bool connect(const ConnectionConfig& config, std::string& errorOut) override;
    void disconnect() override;
    bool isConnected() const override;
    bool ping(std::string& errorOut) override;
    const ConnectionConfig& getConfig() const override { return config_; }

    QueryResult executeQuery(const std::string& sql) override;
    std::vector<std::string> getDatabases(std::string& errorOut) override;
    std::vector<std::string> getTables(const std::string& dbName, std::string& errorOut) override;
    QueryResult getTableStructure(const std::string& dbName, const std::string& tableName, std::string& errorOut) override;
    bool selectDatabase(const std::string& dbName, std::string& errorOut) override;

private:
    MYSQL* mysql_{nullptr};
    bool connected_{false};
    ConnectionConfig config_;
};

} // namespace dbterm

#endif // DBTERM_MYSQLCONNECTION_H
