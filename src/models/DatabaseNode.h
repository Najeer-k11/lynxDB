#ifndef DBTERM_DATABASENODE_H
#define DBTERM_DATABASENODE_H

#include <string>
#include <vector>
#include <memory>

namespace dbterm {

enum class NodeType {
    SERVER,
    DATABASE,
    TABLE_GROUP,
    TABLE,
    VIEW,
    PROCEDURE
};

struct DatabaseNode {
    std::string name;
    NodeType type{NodeType::SERVER};
    bool expanded{false};
    bool loaded{false};
    std::string dbName; // Parent database name
    std::vector<std::shared_ptr<DatabaseNode>> children;

    std::string getPrefix() const {
        switch (type) {
            case NodeType::SERVER:
                return expanded ? "▼ " : "▶ ";
            case NodeType::DATABASE:
                return expanded ? "  ▼ " : "  ▶ ";
            case NodeType::TABLE_GROUP:
                return expanded ? "    ▼ " : "    ▶ ";
            case NodeType::TABLE:
                return "      • ";
            case NodeType::VIEW:
                return "      ◇ ";
            case NodeType::PROCEDURE:
                return "      ƒ ";
            default:
                return "  ";
        }
    }
};

} // namespace dbterm

#endif // DBTERM_DATABASENODE_H
