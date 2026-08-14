#include "ui/ErrorDialog.h"
#include <algorithm>
#include <vector>

namespace dbterm {

ErrorDialog::~ErrorDialog() {
    destroy();
}

void ErrorDialog::init(int termHeight, int termWidth, const std::string& title, const std::string& message) {
    destroy();
    title_ = title;
    message_ = message;

    width_ = std::min(70, std::max(45, termWidth - 8));
    height_ = 8;
    starty_ = (termHeight - height_) / 2;
    if (starty_ < 0) starty_ = 0;
    startx_ = (termWidth - width_) / 2;
    if (startx_ < 0) startx_ = 0;

    win_ = newwin(height_, width_, starty_, startx_);
}

void ErrorDialog::destroy() {
    if (win_) {
        delwin(win_);
        win_ = nullptr;
    }
}

void ErrorDialog::render() {
    if (!win_) return;

    werase(win_);

    box(win_, 0, 0);

    if (has_colors()) {
        wattron(win_, A_BOLD);
    }
    mvwprintw(win_, 0, 2, " %s ", title_.c_str());
    if (has_colors()) {
        wattroff(win_, A_BOLD);
    }

    // Wrap message into 2 lines if necessary
    int maxLineLen = width_ - 6;
    std::string line1 = message_;
    std::string line2;
    if (static_cast<int>(line1.length()) > maxLineLen) {
        size_t splitPos = line1.find_last_of(" \t", maxLineLen);
        if (splitPos != std::string::npos && splitPos > 10) {
            line2 = line1.substr(splitPos + 1);
            line1 = line1.substr(0, splitPos);
        } else {
            line2 = line1.substr(maxLineLen);
            line1 = line1.substr(0, maxLineLen);
        }
    }
    if (static_cast<int>(line2.length()) > maxLineLen) {
        line2 = line2.substr(0, maxLineLen - 3) + "...";
    }

    mvwprintw(win_, 2, 3, "%s", line1.c_str());
    if (!line2.empty()) {
        mvwprintw(win_, 3, 3, "%s", line2.c_str());
    }

    mvwprintw(win_, height_ - 2, 3, "Press [Enter] or [Esc] to dismiss");

    wrefresh(win_);
}

void ErrorDialog::handleInput(int ch, bool& dismissed) {
    dismissed = false;
    if (ch == 27 || ch == '\n' || ch == KEY_ENTER || ch == ' ') {
        dismissed = true;
    }
}

} // namespace dbterm
