#ifndef DBTERM_DATABASECONNECTION_H
#define DBTERM_DATABASECONNECTION_H

#include "models/ConnectionConfig.h"
#include "database/QueryResult.h"
#include <string>
#include <vector>
#include <memory>

namespace dbterm {

struct ForeignKeyInfo {
    std::string fromTable;
    std::string fromColumn;
    std::string toTable;
    std::string toColumn;
};

class DatabaseConnection {
public:
    virtual ~DatabaseConnection() = default;

    virtual bool connect(const ConnectionConfig& config, std::string& errorOut) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual bool ping(std::string& errorOut) = 0;
    virtual const ConnectionConfig& getConfig() const = 0;

    virtual QueryResult executeQuery(const std::string& sql) = 0;
    virtual std::vector<std::string> getDatabases(std::string& errorOut) = 0;
    virtual std::vector<std::string> getTables(const std::string& dbName, std::string& errorOut) = 0;
    virtual QueryResult getTableStructure(const std::string& dbName, const std::string& tableName, std::string& errorOut) = 0;
    virtual std::vector<ForeignKeyInfo> getForeignKeys(const std::string& dbName, const std::string& tableName, std::string& errorOut) = 0;
    virtual bool selectDatabase(const std::string& dbName, std::string& errorOut) = 0;
};

} // namespace dbterm

#endif // DBTERM_DATABASECONNECTION_H
