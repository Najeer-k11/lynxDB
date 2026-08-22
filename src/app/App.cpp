#include "app/App.h"
#include "ui/Screen.h"
#include "models/ConfigManager.h"
#include "models/ThemeManager.h"

#include "ui/CursesCompat.h"
#include <clocale>
#include <algorithm>
#include <iostream>
#include <sstream>

namespace dbterm {

static std::string base64Encode(const std::string& input) {
    static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int val = 0, valb = -6;
    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(table[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(table[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

App::App() {
    initNcurses();
    setupInitialTree();
}

App::~App() {
    cleanupNcurses();
}

void App::cycleNeonTheme() {
    const auto& themes = ThemeManager::getThemes();
    state_.currentThemeIndex = (state_.currentThemeIndex + 1) % static_cast<int>(themes.size());
    ThemeManager::saveThemeIndex(state_.currentThemeIndex);
    ThemeManager::applyTheme(state_.currentThemeIndex);

    clear();
    refresh();

    state_.statusMessage = "Switched Neon Accent Theme to: " + themes[state_.currentThemeIndex].name;
}

void App::setupInitialTree() {
    state_.isConnected = false;
    state_.viewMode = ViewMode::WELCOME;

    auto root = std::make_shared<DatabaseNode>();
    root->name = "Connections";
    root->type = NodeType::SERVER;
    root->expanded = true;

    auto savedConfigs = ConfigManager::loadConnections();
    if (!savedConfigs.empty()) {
        for (const auto& cfg : savedConfigs) {
            auto node = std::make_shared<DatabaseNode>();
            node->name = cfg.name + " (" + cfg.host + ")";
            node->type = NodeType::DATABASE;
            root->children.push_back(node);
        }
        state_.statusMessage = "Welcome! Loaded " + std::to_string(savedConfigs.size()) + " saved connections. Press Enter or 1.." + std::to_string(std::min(9, (int)savedConfigs.size())) + " to Connect.";
    } else {
        auto node = std::make_shared<DatabaseNode>();
        node->name = "Press 'c' or 'n' to Connect";
        node->type = NodeType::DATABASE;
        root->children.push_back(node);
        state_.statusMessage = "Welcome to lynxDB! Press 'c' or 'n' to Connect.";
    }

    state_.treeRoot = root;
    rebuildVisibleTreeNodes();
}

void App::flattenNode(const std::shared_ptr<DatabaseNode>& node, int depth) {
    if (!node) return;
    state_.visibleTreeNodes.push_back({node, depth});
    if (node->expanded) {
        for (const auto& child : node->children) {
            flattenNode(child, depth + 1);
        }
    }
}

void App::rebuildVisibleTreeNodes() {
    state_.visibleTreeNodes.clear();
    if (state_.treeRoot) {
        flattenNode(state_.treeRoot, 0);
    }
}

void App::toggleColumnSort() {
    if (state_.contentHeaders.empty() || state_.contentRows.empty()) return;

    int col = state_.contentSelectedColIndex;
    if (col < 0 || col >= static_cast<int>(state_.contentHeaders.size())) return;

    if (state_.sortColIndex == col) {
        state_.sortAscending = !state_.sortAscending;
    } else {
        state_.sortColIndex = col;
        state_.sortAscending = true;
    }

    auto sortLambda = [col, this](const std::vector<std::string>& a, const std::vector<std::string>& b) {
        std::string valA = (col < static_cast<int>(a.size())) ? a[col] : "";
        std::string valB = (col < static_cast<int>(b.size())) ? b[col] : "";

        bool isNumA = !valA.empty() && std::all_of(valA.begin(), valA.end(), [](char c){ return std::isdigit(c) || c == '-'; });
        bool isNumB = !valB.empty() && std::all_of(valB.begin(), valB.end(), [](char c){ return std::isdigit(c) || c == '-'; });

        if (isNumA && isNumB) {
            try {
                long long numA = std::stoll(valA);
                long long numB = std::stoll(valB);
                return state_.sortAscending ? (numA < numB) : (numA > numB);
            } catch (...) {}
        }
        return state_.sortAscending ? (valA < valB) : (valA > valB);
    };

    std::sort(state_.contentRows.begin(), state_.contentRows.end(), sortLambda);
    if (state_.isFilterActive) {
        std::sort(state_.filteredRows.begin(), state_.filteredRows.end(), sortLambda);
    }

    state_.statusMessage = "Sorted by column '" + state_.contentHeaders[col] + "' (" + (state_.sortAscending ? "ASC" : "DESC") + ").";
}

void App::triggerGoToRow(Screen& screen) {
    const auto& rows = state_.isFilterActive ? state_.filteredRows : state_.contentRows;
    if (rows.empty()) {
        state_.statusMessage = "No rows loaded to jump.";
        return;
    }
    screen.goToRowDialog().init(0, 0, static_cast<int>(rows.size()));
    state_.activeDialog = DialogType::GO_TO_ROW;
}

void App::triggerErDiagram(Screen& screen) {
    if (!state_.isConnected || !dbManager_.isConnected()) {
        state_.statusMessage = "Connect to a database first to view ER diagram.";
        return;
    }
    screen.erDiagramView().loadSchema(dbManager_.activeConnection(), state_.activeDatabaseName);
    state_.viewMode = ViewMode::ER_DIAGRAM;
    state_.statusMessage = "Showing TUI ASCII ER Diagram for database '" + state_.activeDatabaseName + "'.";
}

void App::triggerForeignKeyJump() {
    if (!state_.isConnected || state_.activePanel != Panel::CONTENT || state_.contentHeaders.empty() || state_.contentRows.empty()) {
        state_.statusMessage = "Select a cell in Content Panel to jump via foreign key.";
        return;
    }

    int rowIdx = state_.contentSelectedIndex;
    int colIdx = state_.contentSelectedColIndex;
    const auto& rows = state_.isFilterActive ? state_.filteredRows : state_.contentRows;
    if (rowIdx < 0 || rowIdx >= static_cast<int>(rows.size()) ||
        colIdx < 0 || colIdx >= static_cast<int>(state_.contentHeaders.size())) {
        return;
    }

    std::string colName = state_.contentHeaders[colIdx];
    std::string cellVal = rows[rowIdx][colIdx];
    if (cellVal.empty() || cellVal == "NULL") {
        state_.statusMessage = "Selected cell is NULL / empty. Cannot jump.";
        return;
    }

    std::string targetTable;
    std::string targetCol = "id";

    if (dbManager_.isConnected()) {
        std::string err;
        auto fks = dbManager_.activeConnection()->getForeignKeys(state_.activeDatabaseName, state_.activeTableName, err);
        for (const auto& fk : fks) {
            if (fk.fromColumn == colName) {
                targetTable = fk.toTable;
                targetCol = fk.toColumn;
                break;
            }
        }
    }

    if (targetTable.empty()) {
        // Heuristic fallback: e.g. user_id -> table "users" or "user", PK "id"
        if (colName.length() > 3 && colName.rfind("_id") == colName.length() - 3) {
            std::string base = colName.substr(0, colName.length() - 3);
            std::string err;
            auto tables = dbManager_.activeConnection()->getTables(state_.activeDatabaseName, err);
            for (const auto& tbl : tables) {
                if (tbl == base || tbl == base + "s" || tbl == base + "es") {
                    targetTable = tbl;
                    break;
                }
            }
        }
    }

    if (targetTable.empty()) {
        state_.statusMessage = "No foreign key relationship found for column '" + colName + "'.";
        return;
    }

    state_.statusMessage = "Jumping FK: " + state_.activeTableName + "." + colName + " -> " + targetTable + "." + targetCol + " (" + cellVal + ")";
    loadTableData(state_.activeDatabaseName, targetTable);

    state_.filterQuery = cellVal;
    applyFilter();
}

void App::triggerExport(Screen& screen) {
    if (state_.contentHeaders.empty()) {
        state_.statusMessage = "No data loaded to export.";
        return;
    }
    screen.exportDialog().init(0, 0, state_.activeTableName);
    state_.activeDialog = DialogType::EXPORT_PROMPT;
}

void App::triggerRowInsert(Screen& screen) {
    if (!state_.isConnected || state_.activeTableName.empty() || state_.contentHeaders.empty()) {
        state_.statusMessage = "Select an active table to insert rows.";
        return;
    }
    screen.rowInsertDialog().init(0, 0, state_.contentHeaders);
    state_.activeDialog = DialogType::ROW_INSERT;
}

void App::triggerRowDelete(Screen& screen) {
    if (!state_.isConnected || state_.activePanel != Panel::CONTENT || state_.contentHeaders.empty() || state_.contentRows.empty()) {
        state_.statusMessage = "Select a row in Content Panel to delete.";
        return;
    }

    const auto& rows = state_.isFilterActive ? state_.filteredRows : state_.contentRows;
    int rowIdx = state_.contentSelectedIndex;
    if (rowIdx < 0 || rowIdx >= static_cast<int>(rows.size())) return;

    std::string pkCol;
    std::string pkVal;

    if (dbManager_.isConnected()) {
        std::string err;
        QueryResult structQr = dbManager_.activeConnection()->getTableStructure(state_.activeDatabaseName, state_.activeTableName, err);
        if (structQr.success) {
            for (const auto& r : structQr.rows) {
                if (r.size() >= 4 && (r[3] == "PRI" || r[0] == "id")) {
                    pkCol = r[0];
                    break;
                }
            }
        }
    }

    if (pkCol.empty()) {
        for (const auto& h : state_.contentHeaders) {
            if (h == "id" || h == "ID" || h == "id_pk") {
                pkCol = h;
                break;
            }
        }
    }

    if (pkCol.empty()) {
        state_.errorMessage = "Cannot delete row: Table '" + state_.activeTableName + "' has no primary key column.";
        state_.activeDialog = DialogType::ERROR_POPUP;
        return;
    }

    int pkColIdx = -1;
    for (int i = 0; i < static_cast<int>(state_.contentHeaders.size()); ++i) {
        if (state_.contentHeaders[i] == pkCol) {
            pkColIdx = i;
            break;
        }
    }

    if (pkColIdx == -1 || pkColIdx >= static_cast<int>(rows[rowIdx].size())) {
        state_.errorMessage = "Cannot delete row: Primary key column '" + pkCol + "' not found in target row.";
        state_.activeDialog = DialogType::ERROR_POPUP;
        return;
    }

    pkVal = rows[rowIdx][pkColIdx];

    std::string deleteSql;
    if (dbManager_.activeConfig() && dbManager_.activeConfig()->type == DatabaseType::SQLITE) {
        deleteSql = "DELETE FROM \"" + state_.activeTableName + "\" WHERE \"" + pkCol + "\" = '" + pkVal + "';";
    } else {
        deleteSql = "DELETE FROM `" + state_.activeTableName + "` WHERE `" + pkCol + "` = '" + pkVal + "';";
    }

    state_.pendingUpdateSql = deleteSql;
    screen.confirmDialog().init(0, 0, deleteSql);
    state_.activeDialog = DialogType::CONFIRM_UPDATE;
}

void App::triggerSchemaDdl(DdlAction action, Screen& screen, const std::string& targetTable, const std::string& targetCol) {
    if (!state_.isConnected) {
        state_.statusMessage = "Connect to a database to perform Schema DDL operations.";
        return;
    }
    std::string tbl = targetTable.empty() ? state_.activeTableName : targetTable;
    screen.schemaDdlDialog().init(0, 0, action, tbl, targetCol);
    state_.activeDialog = DialogType::SCHEMA_DDL;
}

void App::applyFilter() {
    if (state_.filterQuery.empty()) {
        state_.isFilterActive = false;
        state_.filteredRows.clear();
        state_.statusMessage = "Filter cleared.";
        return;
    }

    std::string needle = state_.filterQuery;
    std::transform(needle.begin(), needle.end(), needle.begin(), ::tolower);

    state_.filteredRows.clear();
    for (const auto& row : state_.contentRows) {
        bool match = false;
        for (const auto& col : row) {
            std::string haystack = col;
            std::transform(haystack.begin(), haystack.end(), haystack.begin(), ::tolower);
            if (haystack.find(needle) != std::string::npos) {
                match = true;
                break;
            }
        }
        if (match) {
            state_.filteredRows.push_back(row);
        }
    }

    state_.isFilterActive = true;
    state_.contentSelectedIndex = 0;
    state_.contentSelectedColIndex = 0;
    state_.colOffset = 0;

    state_.statusMessage = "Filter active: \"" + state_.filterQuery + "\" (" + std::to_string(state_.filteredRows.size()) + " of " + std::to_string(state_.contentRows.size()) + " rows matching).";
}

void App::triggerSavedConnection(int index, Screen& screen) {
    auto saved = ConfigManager::loadConnections();
    if (index < 0 || index >= static_cast<int>(saved.size())) return;

    ConnectionConfig cfg = saved[index];
    std::string err;

    state_.statusMessage = "Connecting to " + cfg.getDisplayURI() + "...";
    screen.render(state_);

    if (dbManager_.connect(cfg, err)) {
        state_.isConnected = true;
        state_.viewMode = ViewMode::TABLE_DATA;
        state_.activeDialog = DialogType::NONE;
        state_.statusMessage = "Connected to " + cfg.getDisplayURI();

        auto serverNode = std::make_shared<DatabaseNode>();
        std::string label = (cfg.type == DatabaseType::SQLITE) ? "sqlite: " + cfg.host : cfg.host + ":" + std::to_string(cfg.port);
        serverNode->name = label;
        serverNode->type = NodeType::SERVER;
        serverNode->expanded = true;

        std::string dbErr;
        auto dbs = dbManager_.activeConnection()->getDatabases(dbErr);
        for (const auto& db : dbs) {
            auto dbNode = std::make_shared<DatabaseNode>();
            dbNode->name = db;
            dbNode->dbName = db;
            dbNode->type = NodeType::DATABASE;
            serverNode->children.push_back(dbNode);
        }

        state_.treeRoot = serverNode;
        state_.sidebarSelectedIndex = 0;
        rebuildVisibleTreeNodes();
    } else {
        state_.errorMessage = err;
        state_.activeDialog = DialogType::ERROR_POPUP;
        state_.statusMessage = "Connection failed: " + err;
    }
}

void App::loadTableData(const std::string& dbName, const std::string& tableName) {
    state_.activeDatabaseName = dbName;
    state_.activeTableName = tableName;
    state_.viewMode = ViewMode::TABLE_DATA;

    if (dbManager_.isConnected()) {
        std::string err;
        if (!dbName.empty()) {
            dbManager_.activeConnection()->selectDatabase(dbName, err);
        }
        std::string sql = "SELECT * FROM `" + tableName + "` LIMIT 100;";
        if (dbManager_.activeConfig() && dbManager_.activeConfig()->type == DatabaseType::SQLITE) {
            sql = "SELECT * FROM \"" + tableName + "\" LIMIT 100;";
        }
        QueryResult qr = dbManager_.activeConnection()->executeQuery(sql);
        if (qr.success) {
            state_.contentHeaders = qr.columns;
            state_.contentRows = qr.rows;
            state_.contentSelectedIndex = 0;
            state_.contentSelectedColIndex = 0;
            state_.colOffset = 0;
            state_.sortColIndex = -1;
            state_.statusMessage = "Loaded data for '" + tableName + "' (" + std::to_string(qr.rows.size()) + " rows).";

            if (state_.isFilterActive) {
                applyFilter();
            }
        } else {
            state_.statusMessage = "Error loading table data: " + qr.errorMessage;
        }
    }
}

void App::loadTableStructure(const std::string& dbName, const std::string& tableName) {
    state_.activeDatabaseName = dbName;
    state_.activeTableName = tableName;
    state_.viewMode = ViewMode::TABLE_STRUCTURE;

    if (dbManager_.isConnected()) {
        std::string err;
        QueryResult qr = dbManager_.activeConnection()->getTableStructure(dbName, tableName, err);
        if (qr.success) {
            state_.contentHeaders = qr.columns;
            state_.contentRows = qr.rows;
            state_.contentSelectedIndex = 0;
            state_.contentSelectedColIndex = 0;
            state_.colOffset = 0;
            state_.sortColIndex = -1;
            state_.statusMessage = "Loaded schema structure for '" + tableName + "'.";

            if (state_.isFilterActive) {
                applyFilter();
            }
        } else {
            state_.statusMessage = "Error loading schema: " + qr.errorMessage;
        }
    }
}

void App::toggleNodeExpansion(std::shared_ptr<DatabaseNode> node) {
    if (!node) return;

    if (node->type == NodeType::TABLE) {
        loadTableData(node->dbName, node->name);
        state_.activePanel = Panel::CONTENT;
        return;
    }

    if (node->type == NodeType::DATABASE && !node->loaded && dbManager_.isConnected()) {
        std::string err;
        auto tables = dbManager_.activeConnection()->getTables(node->name, err);
        node->children.clear();
        for (const auto& tbl : tables) {
            auto tblNode = std::make_shared<DatabaseNode>();
            tblNode->name = tbl;
            tblNode->type = NodeType::TABLE;
            tblNode->dbName = node->name;
            node->children.push_back(tblNode);
        }
        node->loaded = true;
    }

    if (node->type == NodeType::SERVER || node->type == NodeType::DATABASE || node->type == NodeType::TABLE_GROUP) {
        node->expanded = !node->expanded;
        rebuildVisibleTreeNodes();
    }
}

void App::copyToClipboard(const std::string& text) {
    state_.yankBuffer = text;
    std::string b64 = base64Encode(text);
    std::cout << "\033]52;c;" << b64 << "\007" << std::flush;

    std::string disp = text.empty() ? "(empty)" : text;
    if (disp.length() > 30) disp = disp.substr(0, 27) + "...";
    state_.statusMessage = "Copied to clipboard: \"" + disp + "\"";
}

void App::triggerCellEdit(Screen& screen) {
    if (!state_.isConnected || state_.activePanel != Panel::CONTENT || state_.contentHeaders.empty() || state_.contentRows.empty()) {
        state_.statusMessage = "Select a valid cell in Content Panel to edit.";
        return;
    }

    int rowIdx = state_.contentSelectedIndex;
    int colIdx = state_.contentSelectedColIndex;
    if (rowIdx < 0 || rowIdx >= static_cast<int>(state_.contentRows.size()) ||
        colIdx < 0 || colIdx >= static_cast<int>(state_.contentHeaders.size())) {
        return;
    }

    std::string pkCol;
    std::string pkVal;

    if (dbManager_.isConnected()) {
        std::string err;
        QueryResult structQr = dbManager_.activeConnection()->getTableStructure(state_.activeDatabaseName, state_.activeTableName, err);
        if (structQr.success) {
            for (const auto& row : structQr.rows) {
                if (row.size() >= 4 && (row[3] == "PRI" || row[0] == "id")) {
                    pkCol = row[0];
                    break;
                }
            }
        }
    }

    if (pkCol.empty()) {
        for (const auto& h : state_.contentHeaders) {
            if (h == "id" || h == "ID" || h == "id_pk") {
                pkCol = h;
                break;
            }
        }
    }

    if (pkCol.empty()) {
        state_.errorMessage = "Cannot edit cell: Table '" + state_.activeTableName + "' has no primary key to uniquely identify the row.";
        state_.activeDialog = DialogType::ERROR_POPUP;
        return;
    }

    int pkColIdx = -1;
    for (int i = 0; i < static_cast<int>(state_.contentHeaders.size()); ++i) {
        if (state_.contentHeaders[i] == pkCol) {
            pkColIdx = i;
            break;
        }
    }

    const auto& rows = state_.isFilterActive ? state_.filteredRows : state_.contentRows;
    if (pkColIdx == -1 || pkColIdx >= static_cast<int>(rows[rowIdx].size())) {
        state_.errorMessage = "Cannot edit cell: Primary key column '" + pkCol + "' not found in active row.";
        state_.activeDialog = DialogType::ERROR_POPUP;
        return;
    }

    pkVal = rows[rowIdx][pkColIdx];

    state_.editTargetColumn = state_.contentHeaders[colIdx];
    state_.editTargetPkCol = pkCol;
    state_.editTargetPkVal = pkVal;

    std::string curVal = rows[rowIdx][colIdx];
    screen.cellEditDialog().init(0, 0, state_.editTargetColumn, curVal);
    state_.activeDialog = DialogType::CELL_EDIT;
}

void App::initNcurses() {
    std::setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    if (has_colors()) {
        start_color();
        use_default_colors();
    }

    state_.currentThemeIndex = ThemeManager::loadSavedThemeIndex();
    ThemeManager::applyTheme(state_.currentThemeIndex);
}

void App::cleanupNcurses() {
    endwin();
}

void App::handleInput(int ch) {
    if (state_.viewMode == ViewMode::ER_DIAGRAM) {
        return; // Handled in run loop via erDiagramView
    }

    if (state_.activeDialog == DialogType::NONE) {
        if (!state_.isConnected && state_.viewMode == ViewMode::WELCOME) {
            auto saved = ConfigManager::loadConnections();
            if (ch >= '1' && ch <= '9') {
                int idx = ch - '1';
                if (idx < static_cast<int>(saved.size())) {
                    state_.welcomeSelectedIndex = idx;
                }
            } else if (ch == 'x' || ch == 'X' || ch == KEY_DC) {
                if (!saved.empty() && state_.welcomeSelectedIndex < static_cast<int>(saved.size())) {
                    std::string removedName = saved[state_.welcomeSelectedIndex].name;
                    ConfigManager::removeConnection(state_.welcomeSelectedIndex);
                    auto updated = ConfigManager::loadConnections();
                    if (state_.welcomeSelectedIndex >= static_cast<int>(updated.size())) {
                        state_.welcomeSelectedIndex = std::max(0, static_cast<int>(updated.size()) - 1);
                    }
                    state_.statusMessage = "Removed connection '" + removedName + "'.";
                    setupInitialTree();
                    return;
                }
            }
        }

        switch (ch) {
            case 'q':
            case 'Q':
            case 3: // Ctrl+C
                state_.running = false;
                break;

            case 'a':
            case 'A':
            case KEY_F(2):
                cycleNeonTheme();
                break;

            case 'c':
            case 'C':
            case 'n':
                state_.activeDialog = DialogType::CONNECTION;
                state_.statusMessage = "Edit connection parameters and press Enter to connect.";
                break;

            case 'o':
            case 'O':
                toggleColumnSort();
                break;

            case '/':
                if (state_.isConnected) {
                    state_.activeDialog = DialogType::FILTER_PROMPT;
                    state_.statusMessage = "Type search filter string and press Enter.";
                }
                break;

            case ':':
                state_.activeDialog = DialogType::SQL_PROMPT;
                state_.statusMessage = "Enter SQL query and press F5 to execute.";
                break;

            case 's':
            case 'S':
                if (!state_.activeTableName.empty()) {
                    loadTableStructure(state_.activeDatabaseName, state_.activeTableName);
                } else {
                    state_.statusMessage = "Select a table first to view structure.";
                }
                break;

            case 't':
            case 'T':
                if (!state_.activeTableName.empty()) {
                    loadTableData(state_.activeDatabaseName, state_.activeTableName);
                } else {
                    state_.statusMessage = "Select a table first to view data.";
                }
                break;

            case '\t':
                if (state_.activePanel == Panel::SIDEBAR) {
                    state_.activePanel = Panel::CONTENT;
                    state_.statusMessage = "Active: Content Panel. Nav ↑↓/←→, / Filter, f FK Jump, g GoTo, e Edit, d Del, i Ins, E Exp.";
                } else {
                    state_.activePanel = Panel::SIDEBAR;
                    state_.statusMessage = "Active: Sidebar Panel. Nav ↑↓ tree nodes, Enter expand.";
                }
                break;

            case KEY_LEFT:
            case 'h':
                if (state_.activePanel == Panel::SIDEBAR) {
                    if (state_.sidebarSelectedIndex >= 0 &&
                        state_.sidebarSelectedIndex < static_cast<int>(state_.visibleTreeNodes.size())) {
                        auto node = state_.visibleTreeNodes[state_.sidebarSelectedIndex].node;
                        if (node && node->expanded) {
                            node->expanded = false;
                            rebuildVisibleTreeNodes();
                        }
                    }
                } else {
                    if (state_.contentSelectedColIndex > 0) {
                        state_.contentSelectedColIndex--;
                        if (state_.contentSelectedColIndex < state_.colOffset) {
                            state_.colOffset = state_.contentSelectedColIndex;
                        }
                    }
                }
                break;

            case KEY_RIGHT:
            case 'l':
                if (state_.activePanel == Panel::SIDEBAR) {
                    if (state_.sidebarSelectedIndex >= 0 &&
                        state_.sidebarSelectedIndex < static_cast<int>(state_.visibleTreeNodes.size())) {
                        toggleNodeExpansion(state_.visibleTreeNodes[state_.sidebarSelectedIndex].node);
                    }
                } else {
                    int numCols = static_cast<int>(state_.contentHeaders.size());
                    if (state_.contentSelectedColIndex + 1 < numCols) {
                        state_.contentSelectedColIndex++;
                        if (state_.contentSelectedColIndex >= state_.colOffset + 3) {
                            state_.colOffset++;
                        }
                    }
                }
                break;

            case '\n':
            case KEY_ENTER:
            case ' ':
                if (!state_.isConnected && state_.viewMode == ViewMode::WELCOME) {
                    auto saved = ConfigManager::loadConnections();
                    if (!saved.empty()) {
                        // Handled in run() loop via triggerSavedConnection
                    }
                } else if (state_.activePanel == Panel::SIDEBAR) {
                    if (state_.sidebarSelectedIndex >= 0 &&
                        state_.sidebarSelectedIndex < static_cast<int>(state_.visibleTreeNodes.size())) {
                        toggleNodeExpansion(state_.visibleTreeNodes[state_.sidebarSelectedIndex].node);
                    }
                }
                break;

            case KEY_UP:
            case 'k':
                if (!state_.isConnected && state_.viewMode == ViewMode::WELCOME) {
                    if (state_.welcomeSelectedIndex > 0) {
                        state_.welcomeSelectedIndex--;
                    }
                } else if (state_.activePanel == Panel::SIDEBAR) {
                    if (state_.sidebarSelectedIndex > 0) {
                        state_.sidebarSelectedIndex--;
                    }
                } else {
                    if (state_.contentSelectedIndex > 0) {
                        state_.contentSelectedIndex--;
                    }
                }
                break;

            case KEY_DOWN:
            case 'j':
                if (!state_.isConnected && state_.viewMode == ViewMode::WELCOME) {
                    auto saved = ConfigManager::loadConnections();
                    if (state_.welcomeSelectedIndex + 1 < static_cast<int>(saved.size())) {
                        state_.welcomeSelectedIndex++;
                    }
                } else if (state_.activePanel == Panel::SIDEBAR) {
                    if (state_.sidebarSelectedIndex + 1 < static_cast<int>(state_.visibleTreeNodes.size())) {
                        state_.sidebarSelectedIndex++;
                    }
                } else {
                    const auto& rows = state_.isFilterActive ? state_.filteredRows : state_.contentRows;
                    if (state_.contentSelectedIndex + 1 < static_cast<int>(rows.size())) {
                        state_.contentSelectedIndex++;
                    }
                }
                break;

            case 'y':
                if (state_.activePanel == Panel::CONTENT) {
                    const auto& rows = state_.isFilterActive ? state_.filteredRows : state_.contentRows;
                    if (!rows.empty()) {
                        int r = state_.contentSelectedIndex;
                        int c = state_.contentSelectedColIndex;
                        if (r >= 0 && r < static_cast<int>(rows.size()) &&
                            c >= 0 && c < static_cast<int>(rows[r].size())) {
                            std::string val = rows[r][c];
                            if (val == "NULL") val = "";
                            copyToClipboard(val);
                        }
                    }
                }
                break;

            case 'Y':
                if (state_.activePanel == Panel::CONTENT) {
                    const auto& rows = state_.isFilterActive ? state_.filteredRows : state_.contentRows;
                    if (!rows.empty()) {
                        int r = state_.contentSelectedIndex;
                        if (r >= 0 && r < static_cast<int>(rows.size())) {
                            std::string rowLine;
                            const auto& rowVec = rows[r];
                            for (size_t i = 0; i < rowVec.size(); ++i) {
                                std::string val = (rowVec[i] == "NULL") ? "" : rowVec[i];
                                rowLine += val;
                                if (i + 1 < rowVec.size()) rowLine += "\t";
                            }
                            copyToClipboard(rowLine);
                        }
                    }
                }
                break;

            case KEY_PPAGE:
                if (state_.activePanel == Panel::SIDEBAR) {
                    state_.sidebarSelectedIndex = std::max(0, state_.sidebarSelectedIndex - 10);
                } else {
                    state_.contentSelectedIndex = std::max(0, state_.contentSelectedIndex - 10);
                }
                break;

            case KEY_NPAGE:
                if (state_.activePanel == Panel::SIDEBAR) {
                    int maxIdx = static_cast<int>(state_.visibleTreeNodes.size()) - 1;
                    state_.sidebarSelectedIndex = std::min(std::max(0, maxIdx), state_.sidebarSelectedIndex + 10);
                } else {
                    const auto& rows = state_.isFilterActive ? state_.filteredRows : state_.contentRows;
                    state_.contentSelectedIndex = std::min(static_cast<int>(rows.size()) - 1, state_.contentSelectedIndex + 10);
                }
                break;

            case 'r':
            case 'R':
                if (!state_.activeTableName.empty()) {
                    if (state_.viewMode == ViewMode::TABLE_STRUCTURE) {
                        loadTableStructure(state_.activeDatabaseName, state_.activeTableName);
                    } else {
                        loadTableData(state_.activeDatabaseName, state_.activeTableName);
                    }
                } else {
                    state_.statusMessage = "Refreshed view.";
                }
                break;

            case KEY_RESIZE:
                state_.statusMessage = "Terminal resized.";
                break;

            default:
                break;
        }
    }
}

void App::run() {
    Screen screen;

    while (state_.running) {
        screen.render(state_);

        int ch = getch();
        if (ch == ERR) continue;

        if (state_.viewMode == ViewMode::ER_DIAGRAM) {
            bool closed = false;
            screen.erDiagramView().handleInput(ch, closed);
            if (closed) {
                state_.viewMode = ViewMode::TABLE_DATA;
                state_.statusMessage = "Closed ER Diagram View.";
            }
            continue;
        }

        if (state_.activeDialog == DialogType::NONE) {
            if (ch == 'E') {
                triggerExport(screen);
                continue;
            } else if (ch == 'i') {
                triggerRowInsert(screen);
                continue;
            } else if (ch == 'd') {
                triggerRowDelete(screen);
                continue;
            } else if (ch == 'N') {
                triggerSchemaDdl(DdlAction::CREATE_TABLE, screen);
                continue;
            } else if (ch == 'f') {
                triggerForeignKeyJump();
                continue;
            } else if (ch == 'g' || ch == 'G') {
                triggerGoToRow(screen);
                continue;
            } else if (ch == 'V') {
                triggerErDiagram(screen);
                continue;
            }
        }

        if (state_.activeDialog == DialogType::NONE && !state_.isConnected && state_.viewMode == ViewMode::WELCOME) {
            auto saved = ConfigManager::loadConnections();
            if (!saved.empty()) {
                if (ch >= '1' && ch <= '9') {
                    int idx = ch - '1';
                    if (idx < static_cast<int>(saved.size())) {
                        triggerSavedConnection(idx, screen);
                        continue;
                    }
                } else if (ch == '\n' || ch == KEY_ENTER || ch == ' ') {
                    triggerSavedConnection(state_.welcomeSelectedIndex, screen);
                    continue;
                }
            }
        }

        if (state_.activeDialog == DialogType::GO_TO_ROW) {
            bool submitted = false;
            bool cancelled = false;
            screen.goToRowDialog().handleInput(ch, submitted, cancelled);

            if (cancelled) {
                state_.activeDialog = DialogType::NONE;
                state_.statusMessage = "Go to line cancelled.";
            } else if (submitted) {
                int target = screen.goToRowDialog().getTargetRow();
                state_.contentSelectedIndex = target - 1;
                state_.activeDialog = DialogType::NONE;
                state_.statusMessage = "Jumped to Row " + std::to_string(target) + ".";
            }
        } else if (state_.activeDialog == DialogType::CONNECTION) {
            bool submitted = false;
            bool cancelled = false;
            screen.connectionDialog().handleInput(ch, submitted, cancelled);

            if (cancelled) {
                state_.activeDialog = DialogType::NONE;
                state_.statusMessage = "Connection cancelled.";
            } else if (submitted) {
                ConnectionConfig cfg = screen.connectionDialog().getConfig();
                std::string err;

                state_.statusMessage = "Connecting to " + cfg.getDisplayURI() + "...";
                screen.render(state_);

                if (dbManager_.connect(cfg, err)) {
                    state_.isConnected = true;
                    state_.viewMode = ViewMode::TABLE_DATA;
                    state_.activeDialog = DialogType::NONE;
                    state_.statusMessage = "Connected to " + cfg.getDisplayURI();

                    ConfigManager::saveOrUpdateConnection(cfg);

                    auto serverNode = std::make_shared<DatabaseNode>();
                    std::string label = (cfg.type == DatabaseType::SQLITE) ? "sqlite: " + cfg.host : cfg.host + ":" + std::to_string(cfg.port);
                    serverNode->name = label;
                    serverNode->type = NodeType::SERVER;
                    serverNode->expanded = true;

                    std::string dbErr;
                    auto dbs = dbManager_.activeConnection()->getDatabases(dbErr);
                    for (const auto& db : dbs) {
                        auto dbNode = std::make_shared<DatabaseNode>();
                        dbNode->name = db;
                        dbNode->dbName = db;
                        dbNode->type = NodeType::DATABASE;
                        serverNode->children.push_back(dbNode);
                    }

                    state_.treeRoot = serverNode;
                    state_.sidebarSelectedIndex = 0;
                    rebuildVisibleTreeNodes();
                } else {
                    state_.errorMessage = err;
                    state_.activeDialog = DialogType::ERROR_POPUP;
                    state_.statusMessage = "Connection failed.";
                }
            }
        } else if (state_.activeDialog == DialogType::FILTER_PROMPT) {
            bool submitted = false;
            bool cancelled = false;
            screen.filterDialog().handleInput(ch, submitted, cancelled);

            if (cancelled) {
                state_.filterQuery = "";
                applyFilter();
                state_.activeDialog = DialogType::NONE;
            } else if (submitted) {
                state_.filterQuery = screen.filterDialog().getFilterQuery();
                applyFilter();
                state_.activeDialog = DialogType::NONE;
            }
        } else if (state_.activeDialog == DialogType::EXPORT_PROMPT) {
            bool submitted = false;
            bool cancelled = false;
            screen.exportDialog().handleInput(ch, submitted, cancelled);

            if (cancelled) {
                state_.activeDialog = DialogType::NONE;
                state_.statusMessage = "Export cancelled.";
            } else if (submitted) {
                std::string path = screen.exportDialog().getFilePath();
                ExportFormat fmt = screen.exportDialog().getFormat();
                std::string err;
                bool ok = false;
                const auto& rows = state_.isFilterActive ? state_.filteredRows : state_.contentRows;

                if (fmt == ExportFormat::CSV) {
                    ok = ExportDialog::exportToCsv(path, state_.contentHeaders, rows, err);
                } else {
                    ok = ExportDialog::exportToJson(path, state_.contentHeaders, rows, err);
                }

                if (ok) {
                    state_.activeDialog = DialogType::NONE;
                    state_.statusMessage = "Successfully exported " + std::to_string(rows.size()) + " rows to '" + path + "'.";
                } else {
                    state_.errorMessage = err;
                    state_.activeDialog = DialogType::ERROR_POPUP;
                }
            }
        } else if (state_.activeDialog == DialogType::ROW_INSERT) {
            bool submitted = false;
            bool cancelled = false;
            screen.rowInsertDialog().handleInput(ch, submitted, cancelled);

            if (cancelled) {
                state_.activeDialog = DialogType::NONE;
                state_.statusMessage = "Row insert cancelled.";
            } else if (submitted) {
                auto vals = screen.rowInsertDialog().getValues();
                std::ostringstream ss;
                bool isSqlite = (dbManager_.activeConfig() && dbManager_.activeConfig()->type == DatabaseType::SQLITE);
                std::string quote = isSqlite ? "\"" : "`";

                ss << "INSERT INTO " << quote << state_.activeTableName << quote << " (";
                for (size_t i = 0; i < state_.contentHeaders.size(); ++i) {
                    ss << quote << state_.contentHeaders[i] << quote;
                    if (i + 1 < state_.contentHeaders.size()) ss << ", ";
                }
                ss << ") VALUES (";
                for (size_t i = 0; i < vals.size(); ++i) {
                    if (vals[i].empty()) {
                        ss << "NULL";
                    } else {
                        ss << "'" << vals[i] << "'";
                    }
                    if (i + 1 < vals.size()) ss << ", ";
                }
                ss << ");";

                std::string insertSql = ss.str();
                state_.pendingUpdateSql = insertSql;
                screen.confirmDialog().init(0, 0, insertSql);
                state_.activeDialog = DialogType::CONFIRM_UPDATE;
            }
        } else if (state_.activeDialog == DialogType::SCHEMA_DDL) {
            bool submitted = false;
            bool cancelled = false;
            screen.schemaDdlDialog().handleInput(ch, submitted, cancelled);

            if (cancelled) {
                state_.activeDialog = DialogType::NONE;
                state_.statusMessage = "Schema DDL cancelled.";
            } else if (submitted) {
                bool isSqlite = (dbManager_.activeConfig() && dbManager_.activeConfig()->type == DatabaseType::SQLITE);
                std::string ddlSql = screen.schemaDdlDialog().generateSql(isSqlite);
                state_.pendingUpdateSql = ddlSql;
                screen.confirmDialog().init(0, 0, ddlSql);
                state_.activeDialog = DialogType::CONFIRM_UPDATE;
            }
        } else if (state_.activeDialog == DialogType::ERROR_POPUP) {
            bool dismissed = false;
            screen.errorDialog().handleInput(ch, dismissed);
            if (dismissed) {
                state_.activeDialog = DialogType::NONE;
                state_.statusMessage = "Ready.";
            }
        } else if (state_.activeDialog == DialogType::SQL_PROMPT) {
            bool submitted = false;
            bool cancelled = false;
            screen.sqlQueryDialog().handleInput(ch, submitted, cancelled);

            if (cancelled) {
                state_.activeDialog = DialogType::NONE;
                state_.statusMessage = "SQL prompt cancelled.";
            } else if (submitted) {
                std::string sql = screen.sqlQueryDialog().getQuery();
                if (dbManager_.isConnected()) {
                    QueryResult qr = dbManager_.activeConnection()->executeQuery(sql);
                    if (qr.success) {
                        state_.isConnected = true;
                        state_.viewMode = ViewMode::TABLE_DATA;
                        if (!qr.columns.empty()) {
                            state_.contentHeaders = qr.columns;
                            state_.contentRows = qr.rows;
                        } else {
                            state_.contentHeaders = {"Result"};
                            state_.contentRows = {{"Query executed successfully. " + std::to_string(qr.affectedRows) + " rows affected."}};
                        }
                        state_.contentSelectedIndex = 0;
                        state_.contentSelectedColIndex = 0;
                        state_.colOffset = 0;
                        state_.activeTableName = "SQL Result";
                        state_.activeDialog = DialogType::NONE;
                        state_.statusMessage = "SQL executed successfully.";

                        if (state_.isFilterActive) {
                            applyFilter();
                        }
                    } else {
                        state_.errorMessage = qr.errorMessage;
                        state_.activeDialog = DialogType::ERROR_POPUP;
                    }
                } else {
                    state_.errorMessage = "Not connected to a database server. Press 'c' to connect first.";
                    state_.activeDialog = DialogType::ERROR_POPUP;
                }
            }
        } else if (state_.activeDialog == DialogType::CELL_EDIT) {
            bool submitted = false;
            bool cancelled = false;
            screen.cellEditDialog().handleInput(ch, submitted, cancelled);

            if (cancelled) {
                state_.activeDialog = DialogType::NONE;
                state_.statusMessage = "Cell edit cancelled.";
            } else if (submitted) {
                std::string newVal = screen.cellEditDialog().getNewValue();
                std::string updateSql;

                if (dbManager_.activeConfig() && dbManager_.activeConfig()->type == DatabaseType::SQLITE) {
                    updateSql = "UPDATE \"" + state_.activeTableName + "\" SET \"" + state_.editTargetColumn + "\" = '" + newVal + "' WHERE \"" + state_.editTargetPkCol + "\" = '" + state_.editTargetPkVal + "';";
                } else {
                    updateSql = "UPDATE `" + state_.activeTableName + "` SET `" + state_.editTargetColumn + "` = '" + newVal + "' WHERE `" + state_.editTargetPkCol + "` = '" + state_.editTargetPkVal + "';";
                }

                state_.pendingUpdateSql = updateSql;
                screen.confirmDialog().init(0, 0, updateSql);
                state_.activeDialog = DialogType::CONFIRM_UPDATE;
            }
        } else if (state_.activeDialog == DialogType::CONFIRM_UPDATE) {
            bool confirmed = false;
            bool cancelled = false;
            screen.confirmDialog().handleInput(ch, confirmed, cancelled);

            if (cancelled) {
                state_.activeDialog = DialogType::NONE;
                state_.statusMessage = "Operation cancelled.";
            } else if (confirmed) {
                if (dbManager_.isConnected()) {
                    QueryResult qr = dbManager_.activeConnection()->executeQuery(state_.pendingUpdateSql);
                    if (qr.success) {
                        state_.activeDialog = DialogType::NONE;
                        state_.statusMessage = "Executed SQL successfully.";
                        if (!state_.activeTableName.empty() && state_.activeTableName != "SQL Result") {
                            loadTableData(state_.activeDatabaseName, state_.activeTableName);
                        }
                    } else {
                        state_.errorMessage = qr.errorMessage;
                        state_.activeDialog = DialogType::ERROR_POPUP;
                    }
                }
            }
        } else {
            if (ch == 'e' || ch == 'E') {
                triggerCellEdit(screen);
            } else {
                handleInput(ch);
            }
        }
    }
}

} // namespace dbterm
