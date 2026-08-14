#include "ui/FilterDialog.h"
#include <cctype>
#include <algorithm>

namespace dbterm {

FilterDialog::~FilterDialog() {
    destroy();
}

void FilterDialog::init(int termHeight, int termWidth) {
    destroy();
    width_ = std::min(64, std::max(44, termWidth - 8));
    height_ = 5;
    starty_ = (termHeight - height_) / 2;
    if (starty_ < 0) starty_ = 0;
    startx_ = (termWidth - width_) / 2;
    if (startx_ < 0) startx_ = 0;

    win_ = newwin(height_, width_, starty_, startx_);
}

void FilterDialog::destroy() {
    if (win_) {
        delwin(win_);
        win_ = nullptr;
    }
}

void FilterDialog::render() {
    if (!win_) return;

    werase(win_);

    if (has_colors()) {
        wattron(win_, COLOR_PAIR(1) | A_BOLD);
    }
    box(win_, 0, 0);
    mvwprintw(win_, 0, 2, " Filter Table Rows ( / ) ");
    if (has_colors()) {
        wattroff(win_, COLOR_PAIR(1) | A_BOLD);
    }

    mvwprintw(win_, 2, 3, "Search:");

    int fieldWidth = width_ - 14;
    std::string displayVal = filterInput_;
    if (static_cast<int>(displayVal.length()) > fieldWidth) {
        displayVal = displayVal.substr(displayVal.length() - fieldWidth);
    }

    if (has_colors()) {
        wattron(win_, COLOR_PAIR(2) | A_BOLD);
    } else {
        wattron(win_, A_REVERSE);
    }
    mvwprintw(win_, 2, 11, "[ %-*s ]", fieldWidth, displayVal.c_str());
    if (has_colors()) {
        wattroff(win_, COLOR_PAIR(2) | A_BOLD);
    } else {
        wattroff(win_, A_REVERSE);
    }

    mvwprintw(win_, height_ - 1, 3, " [Enter] Apply Filter | [Esc] Clear ");

    wrefresh(win_);
}

void FilterDialog::handleInput(int ch, bool& submitted, bool& cancelled) {
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
        if (!filterInput_.empty()) {
            filterInput_.pop_back();
        }
    } else if (std::isprint(ch)) {
        if (filterInput_.length() < 128) {
            filterInput_.push_back(static_cast<char>(ch));
        }
    }
}

} // namespace dbterm
