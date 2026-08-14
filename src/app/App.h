#ifndef DBTERM_APP_H
#define DBTERM_APP_H

#include "database/DatabaseManager.h"
#include "models/ConnectionConfig.h"
#include "models/DatabaseNode.h"
#include <string>
#include <vector>
#include <memory>

namespace dbterm {

class Screen;

enum class Panel {
    SIDEBAR,
    CONTENT
};

enum class DialogType {
    NONE,
    CONNECTION,
    ERROR_POPUP,
    SQL_PROMPT,
    CELL_EDIT,
    CONFIRM_UPDATE
};

enum class ViewMode {
    TABLE_DATA,
    TABLE_STRUCTURE,
    SQL_QUERY
};

struct VisibleNode {
    std::shared_ptr<DatabaseNode> node;
    int depth;
};

struct AppState {
    bool running{true};
    Panel activePanel{Panel::SIDEBAR};
    DialogType activeDialog{DialogType::NONE};
    ViewMode viewMode{ViewMode::TABLE_DATA};
    std::string errorMessage;

    // Database Tree Navigation
    std::shared_ptr<DatabaseNode> treeRoot;
    std::vector<VisibleNode> visibleTreeNodes;
    int sidebarSelectedIndex{0};

    // Active Selection State
    std::string activeDatabaseName;
    std::string activeTableName;

    // Content table view & horizontal scrolling state
    int contentSelectedIndex{0};
    int contentSelectedColIndex{0};
    int colOffset{0};

    std::vector<std::string> contentHeaders{"id", "name", "email", "status"};
    std::vector<std::vector<std::string>> contentRows{
        {"1", "Najeer", "najeer@example.com", "ACTIVE"},
        {"2", "Rahul", "rahul@example.com", "INACTIVE"},
        {"3", "Alice", "alice@example.com", "ACTIVE"},
        {"4", "Bob", "bob@example.com", "PENDING"},
        {"5", "Charlie", "charlie@example.com", "ACTIVE"},
        {"6", "David", "david@example.com", "ACTIVE"},
        {"7", "Eve", "eve@example.com", "INACTIVE"},
        {"8", "Frank", "frank@example.com", "PENDING"}
    };

    // Copy / Edit / Confirmation state
    std::string yankBuffer;
    std::string pendingUpdateSql;
    std::string editTargetColumn;
    std::string editTargetPkCol;
    std::string editTargetPkVal;

    std::string statusMessage{"Ready. Press 'c' Connect | 's' Structure | 't' Data | ':' SQL | Tab Switch."};
};

class App {
public:
    App();
    ~App();

    void run();

private:
    void initNcurses();
    void cleanupNcurses();
    void handleInput(int ch);
    void setupDefaultMockTree();
    void rebuildVisibleTreeNodes();
    void flattenNode(const std::shared_ptr<DatabaseNode>& node, int depth);
    void toggleNodeExpansion(std::shared_ptr<DatabaseNode> node);
    void loadTableData(const std::string& dbName, const std::string& tableName);
    void loadTableStructure(const std::string& dbName, const std::string& tableName);
    void copyToClipboard(const std::string& text);
    void triggerCellEdit(Screen& screen);

    AppState state_;
    DatabaseManager dbManager_;
};

} // namespace dbterm

#endif // DBTERM_APP_H
