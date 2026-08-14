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
    setupDefaultMockTree();

    auto savedConfigs = ConfigManager::loadConnections();
    if (!savedConfigs.empty()) {
        state_.statusMessage = "Loaded " + std::to_string(savedConfigs.size()) + " saved connection configurations.";
    }
}

App::~App() {
    cleanupNcurses();
}

void App::setupDefaultMockTree() {
    auto server = std::make_shared<DatabaseNode>();
    server->name = "localhost:3306";
    server->type = NodeType::SERVER;
    server->expanded = true;

    auto db1 = std::make_shared<DatabaseNode>();
    db1->name = "showroom_db";
    db1->dbName = "showroom_db";
    db1->type = NodeType::DATABASE;
    db1->expanded = true;
    db1->loaded = true;

    auto t1 = std::make_shared<DatabaseNode>(); t1->name = "users"; t1->type = NodeType::TABLE; t1->dbName = "showroom_db";
    auto t2 = std::make_shared<DatabaseNode>(); t2->name = "products"; t2->type = NodeType::TABLE; t2->dbName = "showroom_db";
    auto t3 = std::make_shared<DatabaseNode>(); t3->name = "orders"; t3->type = NodeType::TABLE; t3->dbName = "showroom_db";

    db1->children.push_back(t1);
    db1->children.push_back(t2);
    db1->children.push_back(t3);

    server->children.push_back(db1);

    state_.treeRoot = server;
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
        } else {
            state_.statusMessage = "Error loading table data: " + qr.errorMessage;
        }
    } else {
        state_.statusMessage = "Mock table data viewed for '" + tableName + "'. Connect for live query.";
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
        } else {
            state_.statusMessage = "Error loading schema: " + qr.errorMessage;
        }
    } else {
        state_.contentHeaders = {"Field", "Type", "Null", "Key", "Default", "Extra"};
        state_.contentRows = {
            {"id", "int(11)", "NO", "PRI", "NULL", "auto_increment"},
            {"name", "varchar(255)", "NO", "", "NULL", ""},
            {"email", "varchar(255)", "YES", "UNI", "NULL", ""},
            {"status", "varchar(50)", "NO", "", "ACTIVE", ""}
        };
        state_.contentSelectedIndex = 0;
        state_.contentSelectedColIndex = 0;
        state_.colOffset = 0;
        state_.statusMessage = "Mock structure viewed for '" + tableName + "'.";
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
    if (state_.activePanel != Panel::CONTENT || state_.contentHeaders.empty() || state_.contentRows.empty()) {
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

    // Detect Primary Key column
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

    // Find PK value for current row
    int pkColIdx = -1;
    for (int i = 0; i < static_cast<int>(state_.contentHeaders.size()); ++i) {
        if (state_.contentHeaders[i] == pkCol) {
            pkColIdx = i;
            break;
        }
    }

    if (pkColIdx == -1 || pkColIdx >= static_cast<int>(state_.contentRows[rowIdx].size())) {
        state_.errorMessage = "Cannot edit cell: Primary key column '" + pkCol + "' not found in active row.";
        state_.activeDialog = DialogType::ERROR_POPUP;
        return;
    }

    pkVal = state_.contentRows[rowIdx][pkColIdx];

    state_.editTargetColumn = state_.contentHeaders[colIdx];
    state_.editTargetPkCol = pkCol;
    state_.editTargetPkVal = pkVal;

    std::string curVal = state_.contentRows[rowIdx][colIdx];
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

            case '\t': // Tab becomes SOLE panel switch key
                if (state_.activePanel == Panel::SIDEBAR) {
                    state_.activePanel = Panel::CONTENT;
                    state_.statusMessage = "Active: Content Panel. Use ↑↓/←→ to navigate cells, e to Edit, y to Copy.";
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
                if (state_.activePanel == Panel::SIDEBAR) {
                    if (state_.sidebarSelectedIndex >= 0 &&
                        state_.sidebarSelectedIndex < static_cast<int>(state_.visibleTreeNodes.size())) {
                        toggleNodeExpansion(state_.visibleTreeNodes[state_.sidebarSelectedIndex].node);
                    }
                }
                break;

            case KEY_UP:
            case 'k':
                if (state_.activePanel == Panel::SIDEBAR) {
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
                if (state_.activePanel == Panel::SIDEBAR) {
                    if (state_.sidebarSelectedIndex + 1 < static_cast<int>(state_.visibleTreeNodes.size())) {
                        state_.sidebarSelectedIndex++;
                    }
                } else {
                    if (state_.contentSelectedIndex + 1 < static_cast<int>(state_.contentRows.size())) {
                        state_.contentSelectedIndex++;
                    }
                }
                break;

            case 'y': // Copy cell value
                if (state_.activePanel == Panel::CONTENT && !state_.contentRows.empty()) {
                    int r = state_.contentSelectedIndex;
                    int c = state_.contentSelectedColIndex;
                    if (r >= 0 && r < static_cast<int>(state_.contentRows.size()) &&
                        c >= 0 && c < static_cast<int>(state_.contentRows[r].size())) {
                        std::string val = state_.contentRows[r][c];
                        if (val == "NULL") val = ""; // NULL copies as empty string per convention
                        copyToClipboard(val);
                    }
                }
                break;

            case 'Y': // Copy full row tab-separated
                if (state_.activePanel == Panel::CONTENT && !state_.contentRows.empty()) {
                    int r = state_.contentSelectedIndex;
                    if (r >= 0 && r < static_cast<int>(state_.contentRows.size())) {
                        std::string rowLine;
                        const auto& rowVec = state_.contentRows[r];
                        for (size_t i = 0; i < rowVec.size(); ++i) {
                            std::string val = (rowVec[i] == "NULL") ? "" : rowVec[i];
                            rowLine += val;
                            if (i + 1 < rowVec.size()) rowLine += "\t";
                        }
                        copyToClipboard(rowLine);
                    }
                }
                break;

            case KEY_PPAGE:
                if (state_.activePanel == Panel::CONTENT) {
                    state_.contentSelectedIndex = std::max(0, state_.contentSelectedIndex - 10);
                }
                break;

            case KEY_NPAGE:
                if (state_.activePanel == Panel::CONTENT) {
                    state_.contentSelectedIndex = std::min(static_cast<int>(state_.contentRows.size()) - 1, state_.contentSelectedIndex + 10);
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
                } else {
                    // Mock local state update
                    int r = state_.contentSelectedIndex;
                    int c = state_.contentSelectedColIndex;
                    std::string newVal = screen.cellEditDialog().getNewValue();
                    if (r >= 0 && r < static_cast<int>(state_.contentRows.size()) &&
                        c >= 0 && c < static_cast<int>(state_.contentRows[r].size())) {
                        state_.contentRows[r][c] = newVal;
                    }
                    state_.activeDialog = DialogType::NONE;
                    state_.statusMessage = "Mock cell update applied.";
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
