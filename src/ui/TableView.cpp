#include "ui/TableView.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace dbterm {

TableView::~TableView() {
    destroy();
}

void TableView::init(int height, int width, int starty, int startx) {
    destroy();
    height_ = height;
    width_ = width;
    starty_ = starty;
    startx_ = startx;
    win_ = newwin(height_, width_, starty_, startx_);
}

void TableView::destroy() {
    if (win_) {
        delwin(win_);
        win_ = nullptr;
    }
}

void TableView::render(const AppState& state) {
    if (!win_) return;

    werase(win_);

    bool isActive = (state.activePanel == Panel::CONTENT);

    if (isActive && has_colors()) {
        wattron(win_, COLOR_PAIR(1) | A_BOLD);
    }

    box(win_, 0, 0);
    std::string title = " Content: ";
    if (!state.activeTableName.empty()) {
        title += state.activeDatabaseName.empty() ? "" : (state.activeDatabaseName + ".");
        title += state.activeTableName + " ";
    }

    const auto& displayRows = state.isFilterActive ? state.filteredRows : state.contentRows;
    int totalRows = static_cast<int>(displayRows.size());
    int maxDataRows = height_ - 4;
    int scrollOffset = 0;
    if (state.contentSelectedIndex >= maxDataRows) {
        scrollOffset = state.contentSelectedIndex - maxDataRows + 1;
    }
    int visibleRows = std::min(maxDataRows, totalRows - scrollOffset);

    int startRowDisplay = totalRows > 0 ? (scrollOffset + 1) : 0;
    int endRowDisplay = totalRows > 0 ? (scrollOffset + visibleRows) : 0;

    title += "(Rows " + std::to_string(startRowDisplay) + "-" + std::to_string(endRowDisplay) + " of " + std::to_string(totalRows);
    if (state.isFilterActive) {
        title += " | Filter: \"" + state.filterQuery + "\"";
    }
    if (state.sortColIndex != -1) {
        title += std::string(" | Sorted: ") + (state.sortAscending ? "ASC" : "DESC");
    }
    title += ") ";

    mvwprintw(win_, 0, 2, "%s", title.c_str());

    if (isActive && has_colors()) {
        wattroff(win_, COLOR_PAIR(1) | A_BOLD);
    }

    if (state.contentHeaders.empty() || height_ < 4 || width_ < 10) {
        mvwprintw(win_, 2, 4, "[ No table data loaded. Select a table from Sidebar ]");
        wrefresh(win_);
        return;
    }

    if (displayRows.empty()) {
        mvwprintw(win_, 2, 4, "[ No rows matching filter query: \"%s\" ]", state.filterQuery.c_str());
        wrefresh(win_);
        return;
    }

    int gutterWidth = 5; // e.g. " 123 │"

    int numCols = static_cast<int>(state.contentHeaders.size());
    std::vector<int> colWidths(numCols, 8);
    for (int c = 0; c < numCols; ++c) {
        std::string hdr = state.contentHeaders[c];
        if (c == state.sortColIndex) {
            hdr += (state.sortAscending ? " ▲" : " ▼");
        }
        colWidths[c] = std::max(colWidths[c], static_cast<int>(hdr.length()));
        for (const auto& row : displayRows) {
            if (c < static_cast<int>(row.size())) {
                colWidths[c] = std::max(colWidths[c], static_cast<int>(row[c].length()));
            }
        }
        colWidths[c] = std::min(colWidths[c], 30);
    }

    int availableWidth = width_ - 4 - gutterWidth;
    if (availableWidth < 10) availableWidth = 10;

    int colStart = std::clamp(state.colOffset, 0, std::max(0, numCols - 1));
    int visibleCols = 0;
    int currentWidth = 0;
    for (int c = colStart; c < numCols; ++c) {
        if (currentWidth + colWidths[c] + 1 > availableWidth && visibleCols > 0) break;
        currentWidth += colWidths[c] + 1;
        visibleCols++;
    }
    if (visibleCols == 0) visibleCols = 1;

    std::string headerLine = "    │";
    for (int c = 0; c < visibleCols; ++c) {
        int actualColIdx = colStart + c;
        std::string colName = state.contentHeaders[actualColIdx];
        if (actualColIdx == state.sortColIndex) {
            colName += (state.sortAscending ? " ▲" : " ▼");
        }
        if (static_cast<int>(colName.length()) > colWidths[actualColIdx]) {
            colName = colName.substr(0, colWidths[actualColIdx] - 1) + "~";
        }
        std::ostringstream ss;
        ss << std::left << std::setw(colWidths[actualColIdx]) << colName;
        headerLine += ss.str();
        if (c < visibleCols - 1) headerLine += "│";
    }

    wattron(win_, A_BOLD);
    mvwprintw(win_, 1, 2, "%s", headerLine.c_str());
    wattroff(win_, A_BOLD);

    std::string sepLine = "────┼";
    for (int c = 0; c < visibleCols; ++c) {
        int actualColIdx = colStart + c;
        for (int w = 0; w < colWidths[actualColIdx]; ++w) {
            sepLine += "─";
        }
        if (c < visibleCols - 1) sepLine += "┼";
    }
    mvwprintw(win_, 2, 2, "%s", sepLine.c_str());

    for (int r = 0; r < visibleRows; ++r) {
        int actualRowIdx = scrollOffset + r;
        bool isRowSelected = (actualRowIdx == state.contentSelectedIndex);

        const auto& rowData = displayRows[actualRowIdx];

        if (has_colors()) {
            wattron(win_, COLOR_PAIR(4));
        }
        mvwprintw(win_, r + 3, 2, "%4d│", actualRowIdx + 1);
        if (has_colors()) {
            wattroff(win_, COLOR_PAIR(4));
        }

        int printX = 2 + gutterWidth;
        for (int c = 0; c < visibleCols; ++c) {
            int actualColIdx = colStart + c;
            bool isCellSelected = isRowSelected && (actualColIdx == state.contentSelectedColIndex);

            std::string val = (actualColIdx < static_cast<int>(rowData.size())) ? rowData[actualColIdx] : "";
            if (static_cast<int>(val.length()) > colWidths[actualColIdx]) {
                val = val.substr(0, colWidths[actualColIdx] - 1) + "~";
            }
            std::ostringstream ss;
            ss << std::left << std::setw(colWidths[actualColIdx]) << val;
            std::string cellText = ss.str();

            if (isCellSelected && isActive) {
                if (has_colors()) {
                    wattron(win_, COLOR_PAIR(2) | A_BOLD);
                } else {
                    wattron(win_, A_REVERSE);
                }
            } else if (isRowSelected) {
                wattron(win_, A_UNDERLINE);
            }

            mvwprintw(win_, r + 3, printX, "%s", cellText.c_str());

            if (isCellSelected && isActive) {
                if (has_colors()) {
                    wattroff(win_, COLOR_PAIR(2) | A_BOLD);
                } else {
                    wattroff(win_, A_REVERSE);
                }
            } else if (isRowSelected) {
                wattroff(win_, A_UNDERLINE);
            }

            printX += colWidths[actualColIdx];
            if (c < visibleCols - 1) {
                mvwprintw(win_, r + 3, printX, "│");
                printX += 1;
            }
        }
    }

    wrefresh(win_);
}

} // namespace dbterm
