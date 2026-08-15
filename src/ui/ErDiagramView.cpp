#include "ui/ErDiagramView.h"
#include <algorithm>
#include <sstream>

namespace dbterm {

ErDiagramView::~ErDiagramView() {
    destroy();
}

void ErDiagramView::init(int height, int width, int starty, int startx) {
    destroy();
    height_ = height;
    width_ = width;
    starty_ = starty;
    startx_ = startx;
    win_ = newwin(height_, width_, starty_, startx_);
}

void ErDiagramView::destroy() {
    if (win_) {
        delwin(win_);
        win_ = nullptr;
    }
}

void ErDiagramView::loadSchema(DatabaseConnection* conn, const std::string& dbName) {
    tables_.clear();
    if (!conn || !conn->isConnected()) return;

    dbName_ = dbName;
    std::string err;
    auto tblList = conn->getTables(dbName_, err);

    for (const auto& tbl : tblList) {
        ErTableNode node;
        node.tableName = tbl;

        std::string sErr;
        QueryResult structQr = conn->getTableStructure(dbName_, tbl, sErr);
        if (structQr.success) {
            for (const auto& row : structQr.rows) {
                if (!row.empty()) {
                    std::string cName = row[0];
                    std::string cType = (row.size() > 1) ? row[1] : "";
                    node.columns.push_back({cName, cType});
                    if (row.size() >= 4 && (row[3] == "PRI" || row[0] == "id")) {
                        node.primaryKeys.push_back(cName);
                    }
                }
            }
        }

        std::string fkErr;
        node.foreignKeys = conn->getForeignKeys(dbName_, tbl, fkErr);
        tables_.push_back(node);
    }
    scrollY_ = 0;
    scrollX_ = 0;
}

void ErDiagramView::render() {
    if (!win_) return;

    werase(win_);
    box(win_, 0, 0);

    if (has_colors()) {
        wattron(win_, COLOR_PAIR(1) | A_BOLD);
    }
    mvwprintw(win_, 0, 2, " ER Relationship Viewer: %s ", dbName_.c_str());
    if (has_colors()) {
        wattroff(win_, COLOR_PAIR(1) | A_BOLD);
    }

    if (tables_.empty()) {
        mvwprintw(win_, 2, 4, "[ No tables / relationships found in active database ]");
        wrefresh(win_);
        return;
    }

    int boxWidth = 36;
    int currY = 2 - scrollY_;
    int currX = 3 - scrollX_;

    for (size_t t = 0; t < tables_.size(); ++t) {
        const auto& tbl = tables_[t];
        int boxHeight = static_cast<int>(tbl.columns.size()) + 4;

        if (currY + boxHeight >= 1 && currY < height_ - 2 && currX + boxWidth >= 1 && currX < width_ - 2) {
            if (has_colors()) {
                wattron(win_, COLOR_PAIR(1) | A_BOLD);
            }
            mvwprintw(win_, currY, currX, "┌─ %-*s ┐", boxWidth - 5, tbl.tableName.c_str());
            mvwprintw(win_, currY + 1, currX, "├──────────────────────────────────┤");
            if (has_colors()) {
                wattroff(win_, COLOR_PAIR(1) | A_BOLD);
            }

            int lineIdx = 0;
            for (const auto& col : tbl.columns) {
                std::string badge = "    ";
                bool isPk = std::find(tbl.primaryKeys.begin(), tbl.primaryKeys.end(), col.first) != tbl.primaryKeys.end();
                bool isFk = false;
                std::string refTarget;
                for (const auto& fk : tbl.foreignKeys) {
                    if (fk.fromColumn == col.first) {
                        isFk = true;
                        refTarget = fk.toTable + "." + fk.toColumn;
                        break;
                    }
                }

                if (isPk) badge = "[PK]";
                else if (isFk) badge = "[FK]";

                std::string lineStr = badge + " " + col.first + " (" + col.second + ")";
                if (isFk && !refTarget.empty()) {
                    lineStr += " -> " + refTarget;
                }
                if (static_cast<int>(lineStr.length()) > boxWidth - 4) {
                    lineStr = lineStr.substr(0, boxWidth - 5) + "~";
                }

                mvwprintw(win_, currY + 2 + lineIdx, currX, "│ %-*s │", boxWidth - 4, lineStr.c_str());
                lineIdx++;
            }

            if (has_colors()) {
                wattron(win_, COLOR_PAIR(1));
            }
            mvwprintw(win_, currY + 2 + lineIdx, currX, "└──────────────────────────────────┘");
            if (has_colors()) {
                wattroff(win_, COLOR_PAIR(1));
            }
        }

        currX += boxWidth + 4;
        if (currX + boxWidth > width_ - 4) {
            currX = 3 - scrollX_;
            currY += 12;
        }
    }

    mvwprintw(win_, height_ - 2, 3, "Press [↑↓←→/hjkl] Scroll ER View | [Esc / V] Close ER Viewer");

    wrefresh(win_);
}

void ErDiagramView::handleInput(int ch, bool& closed) {
    closed = false;

    if (ch == 27 || ch == 'v' || ch == 'V' || ch == 'q') {
        closed = true;
        return;
    }

    switch (ch) {
        case KEY_UP:
        case 'k':
            scrollY_ = std::max(0, scrollY_ - 2);
            break;
        case KEY_DOWN:
        case 'j':
            scrollY_ += 2;
            break;
        case KEY_LEFT:
        case 'h':
            scrollX_ = std::max(0, scrollX_ - 4);
            break;
        case KEY_RIGHT:
        case 'l':
            scrollX_ += 4;
            break;
    }
}

} // namespace dbterm
