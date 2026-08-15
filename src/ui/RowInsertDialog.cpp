#include "ui/RowInsertDialog.h"
#include <cctype>
#include <algorithm>

namespace dbterm {

RowInsertDialog::~RowInsertDialog() {
    destroy();
}

void RowInsertDialog::init(int termHeight, int termWidth, const std::vector<std::string>& headers) {
    destroy();
    headers_ = headers;
    values_.assign(headers.size(), "");
    activeFieldIndex_ = 0;

    width_ = std::min(72, std::max(52, termWidth - 8));
    height_ = std::min(14, std::max(8, static_cast<int>(headers.size()) + 5));
    starty_ = (termHeight - height_) / 2;
    if (starty_ < 0) starty_ = 0;
    startx_ = (termWidth - width_) / 2;
    if (startx_ < 0) startx_ = 0;

    win_ = newwin(height_, width_, starty_, startx_);
}

void RowInsertDialog::destroy() {
    if (win_) {
        delwin(win_);
        win_ = nullptr;
    }
}

void RowInsertDialog::render() {
    if (!win_) return;

    werase(win_);

    box(win_, 0, 0);

    if (has_colors()) {
        wattron(win_, COLOR_PAIR(1) | A_BOLD);
    }
    mvwprintw(win_, 0, 2, " Insert New Row ");
    if (has_colors()) {
        wattroff(win_, COLOR_PAIR(1) | A_BOLD);
    }

    int maxVisibleFields = height_ - 4;
    int visibleCount = std::min(maxVisibleFields, static_cast<int>(headers_.size()));
    int startIdx = 0;
    if (activeFieldIndex_ >= maxVisibleFields) {
        startIdx = activeFieldIndex_ - maxVisibleFields + 1;
    }

    for (int i = 0; i < visibleCount; ++i) {
        int fieldIdx = startIdx + i;
        bool isActive = (fieldIdx == activeFieldIndex_);

        std::string label = headers_[fieldIdx];
        if (label.length() > 14) label = label.substr(0, 13) + "~";

        mvwprintw(win_, 2 + i, 3, "%-14s:", label.c_str());

        int fieldWidth = width_ - 22;
        std::string displayVal = values_[fieldIdx];
        if (static_cast<int>(displayVal.length()) > fieldWidth) {
            displayVal = displayVal.substr(displayVal.length() - fieldWidth);
        }

        if (isActive) {
            if (has_colors()) {
                wattron(win_, COLOR_PAIR(2) | A_BOLD);
            } else {
                wattron(win_, A_REVERSE);
            }
        }
        mvwprintw(win_, 2 + i, 19, "[ %-*s ]", fieldWidth, displayVal.c_str());
        if (isActive) {
            if (has_colors()) {
                wattroff(win_, COLOR_PAIR(2) | A_BOLD);
            } else {
                wattroff(win_, A_REVERSE);
            }
        }
    }

    mvwprintw(win_, height_ - 2, 3, "Press [Tab/↓] Next Field | [Shift+Tab/↑] Prev | [F5] Save | [Esc] Cancel");

    wrefresh(win_);
}

void RowInsertDialog::handleInput(int ch, bool& submitted, bool& cancelled) {
    submitted = false;
    cancelled = false;

    if (ch == 27) { // Esc
        cancelled = true;
        return;
    }

    if (ch == KEY_F(5)) {
        submitted = true;
        return;
    }

    if (ch == '\t' || ch == KEY_DOWN) {
        if (!headers_.empty()) {
            activeFieldIndex_ = (activeFieldIndex_ + 1) % headers_.size();
        }
        return;
    }

    if (ch == KEY_BTAB || ch == KEY_UP) {
        if (!headers_.empty()) {
            activeFieldIndex_ = (activeFieldIndex_ - 1 + headers_.size()) % headers_.size();
        }
        return;
    }

    if (ch == '\n' || ch == KEY_ENTER) {
        if (activeFieldIndex_ + 1 < static_cast<int>(headers_.size())) {
            activeFieldIndex_++;
        } else {
            submitted = true;
        }
        return;
    }

    if (activeFieldIndex_ >= 0 && activeFieldIndex_ < static_cast<int>(values_.size())) {
        if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
            if (!values_[activeFieldIndex_].empty()) {
                values_[activeFieldIndex_].pop_back();
            }
        } else if (std::isprint(ch)) {
            if (values_[activeFieldIndex_].length() < 256) {
                values_[activeFieldIndex_].push_back(static_cast<char>(ch));
            }
        }
    }
}

} // namespace dbterm
