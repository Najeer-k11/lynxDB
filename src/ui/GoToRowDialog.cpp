#include "ui/GoToRowDialog.h"
#include <cctype>
#include <cstdlib>
#include <algorithm>

namespace dbterm {

GoToRowDialog::~GoToRowDialog() {
    destroy();
}

void GoToRowDialog::init(int termHeight, int termWidth, int maxRows) {
    destroy();
    maxRows_ = std::max(1, maxRows);
    inputBuffer_ = "1";

    width_ = std::min(52, std::max(36, termWidth - 8));
    height_ = 7;
    starty_ = (termHeight - height_) / 2;
    if (starty_ < 0) starty_ = 0;
    startx_ = (termWidth - width_) / 2;
    if (startx_ < 0) startx_ = 0;

    win_ = newwin(height_, width_, starty_, startx_);
}

void GoToRowDialog::destroy() {
    if (win_) {
        delwin(win_);
        win_ = nullptr;
    }
}

void GoToRowDialog::render() {
    if (!win_) return;

    werase(win_);
    box(win_, 0, 0);

    if (has_colors()) {
        wattron(win_, COLOR_PAIR(1) | A_BOLD);
    }
    mvwprintw(win_, 0, 2, " Go to Line / Row ");
    if (has_colors()) {
        wattroff(win_, COLOR_PAIR(1) | A_BOLD);
    }

    mvwprintw(win_, 2, 3, "Jump to Row (1 .. %d):", maxRows_);

    int fieldWidth = width_ - 8;
    std::string displayVal = inputBuffer_;
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

    mvwprintw(win_, height_ - 2, 3, "Press [Enter] Jump | [Esc] Cancel");

    wrefresh(win_);
}

int GoToRowDialog::getTargetRow() const {
    try {
        int val = std::stoi(inputBuffer_);
        return std::clamp(val, 1, maxRows_);
    } catch (...) {
        return 1;
    }
}

void GoToRowDialog::handleInput(int ch, bool& submitted, bool& cancelled) {
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
        if (!inputBuffer_.empty()) {
            inputBuffer_.pop_back();
        }
    } else if (std::isdigit(ch)) {
        if (inputBuffer_.length() < 8) {
            inputBuffer_.push_back(static_cast<char>(ch));
        }
    }
}

} // namespace dbterm
