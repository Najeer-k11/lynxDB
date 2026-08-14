#include "ui/CellEditDialog.h"
#include <cctype>
#include <algorithm>

namespace dbterm {

CellEditDialog::~CellEditDialog() {
    destroy();
}

void CellEditDialog::init(int termHeight, int termWidth, const std::string& columnName, const std::string& currentValue) {
    destroy();
    columnName_ = columnName;
    editBuffer_ = (currentValue == "NULL") ? "" : currentValue;

    width_ = std::min(64, std::max(44, termWidth - 8));
    height_ = 8;
    starty_ = (termHeight - height_) / 2;
    if (starty_ < 0) starty_ = 0;
    startx_ = (termWidth - width_) / 2;
    if (startx_ < 0) startx_ = 0;

    win_ = newwin(height_, width_, starty_, startx_);
}

void CellEditDialog::destroy() {
    if (win_) {
        delwin(win_);
        win_ = nullptr;
    }
}

void CellEditDialog::render() {
    if (!win_) return;

    werase(win_);

    if (has_colors()) {
        wattron(win_, COLOR_PAIR(1) | A_BOLD);
    }
    box(win_, 0, 0);
    std::string title = " Edit Cell: " + columnName_ + " ";
    mvwprintw(win_, 0, 2, "%s", title.c_str());
    if (has_colors()) {
        wattroff(win_, COLOR_PAIR(1) | A_BOLD);
    }

    mvwprintw(win_, 2, 3, "New Value:");

    int fieldWidth = width_ - 8;
    std::string displayVal = editBuffer_;
    if (static_cast<int>(displayVal.length()) > fieldWidth) {
        displayVal = displayVal.substr(displayVal.length() - fieldWidth);
    }

    if (has_colors()) {
        wattron(win_, COLOR_PAIR(2) | A_BOLD);
    } else {
        wattron(win_, A_REVERSE);
    }
    mvwprintw(win_, 3, 3, "[ %-*s ]", fieldWidth, displayVal.c_str());
    if (has_colors()) {
        wattroff(win_, COLOR_PAIR(2) | A_BOLD);
    } else {
        wattroff(win_, A_REVERSE);
    }

    mvwprintw(win_, height_ - 2, 3, "Press [Enter] Confirm | [Esc] Cancel");

    wrefresh(win_);
}

void CellEditDialog::handleInput(int ch, bool& submitted, bool& cancelled) {
    submitted = false;
    cancelled = false;

    if (ch == 27) { // Esc
        cancelled = true;
        return;
    }

    if (ch == '\n' || ch == KEY_ENTER) {
        submitted = true;
        return;
    }

    if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
        if (!editBuffer_.empty()) {
            editBuffer_.pop_back();
        }
    } else if (std::isprint(ch)) {
        if (editBuffer_.length() < 256) {
            editBuffer_.push_back(static_cast<char>(ch));
        }
    }
}

} // namespace dbterm
