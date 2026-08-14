#include "app/App.h"
#include "ui/Screen.h"
#include "models/ConfigManager.h"

#include <ncursesw/ncurses.h>
#include <clocale>
#include <algorithm>
#include <iostream>

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
        state_.statusMessage = "Connection failed.";
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
        init_pair(1, COLOR_CYAN, -1);
        init_pair(2, COLOR_BLACK, COLOR_CYAN);
        init_pair(3, COLOR_BLACK, COLOR_WHITE);
        init_pair(4, COLOR_WHITE, -1);
    }
}

void App::cleanupNcurses() {
    endwin();
}

void App::handleInput(int ch) {
    if (state_.activeDialog == DialogType::NONE) {
        if (!state_.isConnected && state_.viewMode == ViewMode::WELCOME) {
            auto saved = ConfigManager::loadConnections();
            if (ch >= '1' && ch <= '9') {
                int idx = ch - '1';
                if (idx < static_cast<int>(saved.size())) {
                    state_.welcomeSelectedIndex = idx;
                }
            }
        }

        switch (ch) {
            case 'q':
            case 'Q':
            case 3: // Ctrl+C
                state_.running = false;
                break;

            case 'c':
            case 'C':
            case 'n':
            case 'N':
                state_.activeDialog = DialogType::CONNECTION;
                state_.statusMessage = "Edit connection parameters and press Enter to connect.";
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
                    state_.statusMessage = "Active: Content Panel. Use ↑↓/←→ to navigate cells, / to Filter, e to Edit, y to Copy.";
                } else {
                    state_.activePanel = Panel::SIDEBAR;
                    state_.statusMessage = "Active: Sidebar Panel. Use ↑↓ to navigate tree nodes, Enter to expand.";
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
                        // Will be executed in run() using triggerSavedConnection
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

        if (state_.activeDialog == DialogType::CONNECTION) {
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

                    auto saved = ConfigManager::loadConnections();
                    saved.push_back(cfg);
                    ConfigManager::saveConnections(saved);

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
                state_.statusMessage = "Update cancelled.";
            } else if (confirmed) {
                if (dbManager_.isConnected()) {
                    QueryResult qr = dbManager_.activeConnection()->executeQuery(state_.pendingUpdateSql);
                    if (qr.success) {
                        state_.activeDialog = DialogType::NONE;
                        state_.statusMessage = "Cell update executed successfully.";
                        loadTableData(state_.activeDatabaseName, state_.activeTableName);
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
