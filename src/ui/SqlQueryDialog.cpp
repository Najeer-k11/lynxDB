#include "ui/SqlQueryDialog.h"
#include <cctype>
#include <algorithm>
#include <sstream>
#include <vector>

namespace dbterm {

SqlQueryDialog::~SqlQueryDialog() {
    destroy();
}

void SqlQueryDialog::init(int termHeight, int termWidth) {
    destroy();
    width_ = std::min(80, std::max(54, termWidth - 8));
    height_ = 10;
    starty_ = (termHeight - height_) / 2;
    if (starty_ < 0) starty_ = 0;
    startx_ = (termWidth - width_) / 2;
    if (startx_ < 0) startx_ = 0;

    win_ = newwin(height_, width_, starty_, startx_);
}

void SqlQueryDialog::destroy() {
    if (win_) {
        delwin(win_);
        win_ = nullptr;
    }
}

void SqlQueryDialog::render() {
    if (!win_) return;

    werase(win_);

    if (has_colors()) {
        wattron(win_, COLOR_PAIR(1) | A_BOLD);
    }
    box(win_, 0, 0);
    mvwprintw(win_, 0, 2, " Execute SQL Query ");
    if (has_colors()) {
        wattroff(win_, COLOR_PAIR(1) | A_BOLD);
    }

    // Split input into lines for multiline rendering
    std::stringstream ss(queryInput_);
    std::string line;
    std::vector<std::string> lines;
    while (std::getline(ss, line)) {
        lines.push_back(line);
    }
    if (lines.empty()) lines.push_back("");

    int maxVisibleLines = height_ - 3;
    int visibleCount = std::min(maxVisibleLines, static_cast<int>(lines.size()));
    int startLineIdx = std::max(0, static_cast<int>(lines.size()) - maxVisibleLines);

    for (int i = 0; i < visibleCount; ++i) {
        std::string l = lines[startLineIdx + i];
        int maxLen = width_ - 6;
        if (static_cast<int>(l.length()) > maxLen) {
            l = l.substr(0, maxLen - 3) + "...";
        }
        mvwprintw(win_, 1 + i, 2, "%s", l.c_str());
    }

    mvwprintw(win_, height_ - 1, 2, " [F5 / Ctrl+R] Run Query | [Enter] Newline | [Esc] Cancel ");

    wrefresh(win_);
}

void SqlQueryDialog::handleInput(int ch, bool& submitted, bool& cancelled) {
    submitted = false;
    cancelled = false;

    if (ch == 27) { // Esc key
        cancelled = true;
        return;
    }

    // F5 key or Ctrl+R (18) to execute query
    if (ch == KEY_F(5) || ch == 18) {
        submitted = true;
        return;
    }

    if (ch == '\n' || ch == KEY_ENTER) {
        if (queryInput_.length() < 1024) {
            queryInput_.push_back('\n');
        }
        return;
    }

    if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
        if (!queryInput_.empty()) {
            queryInput_.pop_back();
        }
    } else if (std::isprint(ch)) {
        if (queryInput_.length() < 1024) {
            queryInput_.push_back(static_cast<char>(ch));
        }
    }
}

} // namespace dbterm
