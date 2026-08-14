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
    CONFIRM_UPDATE,
    FILTER_PROMPT
};

enum class ViewMode {
    WELCOME,
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
    bool isConnected{false};
    Panel activePanel{Panel::SIDEBAR};
    DialogType activeDialog{DialogType::NONE};
    ViewMode viewMode{ViewMode::WELCOME};
    std::string errorMessage;

    // Database Tree Navigation
    std::shared_ptr<DatabaseNode> treeRoot;
    std::vector<VisibleNode> visibleTreeNodes;
    int sidebarSelectedIndex{0};
    int welcomeSelectedIndex{0};

    // Active Selection State
    std::string activeDatabaseName;
    std::string activeTableName;

    // Content table view & horizontal scrolling state
    int contentSelectedIndex{0};
    int contentSelectedColIndex{0};
    int colOffset{0};

    std::vector<std::string> contentHeaders;
    std::vector<std::vector<std::string>> contentRows;

    // In-UI Search / Filter state
    std::string filterQuery;
    bool isFilterActive{false};
    std::vector<std::vector<std::string>> filteredRows;

    // Copy / Edit / Confirmation state
    std::string yankBuffer;
    std::string pendingUpdateSql;
    std::string editTargetColumn;
    std::string editTargetPkCol;
    std::string editTargetPkVal;

    std::string statusMessage{"Welcome to lynxDB! Press 'c' to Connect or select a saved connection."};
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
    void setupInitialTree();
    void rebuildVisibleTreeNodes();
    void flattenNode(const std::shared_ptr<DatabaseNode>& node, int depth);
    void toggleNodeExpansion(std::shared_ptr<DatabaseNode> node);
    void loadTableData(const std::string& dbName, const std::string& tableName);
    void loadTableStructure(const std::string& dbName, const std::string& tableName);
    void copyToClipboard(const std::string& text);
    void triggerCellEdit(Screen& screen);
    void triggerSavedConnection(int index, Screen& screen);
    void applyFilter();

    AppState state_;
    DatabaseManager dbManager_;
};

} // namespace dbterm

#endif // DBTERM_APP_H
