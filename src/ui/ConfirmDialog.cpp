#include "ui/ConfirmDialog.h"
#include <algorithm>

namespace dbterm {

ConfirmDialog::~ConfirmDialog() {
    destroy();
}

void ConfirmDialog::init(int termHeight, int termWidth, const std::string& sqlStatement) {
    destroy();
    sqlStatement_ = sqlStatement;

    width_ = std::min(72, std::max(50, termWidth - 8));
    height_ = 8;
    starty_ = (termHeight - height_) / 2;
    if (starty_ < 0) starty_ = 0;
    startx_ = (termWidth - width_) / 2;
    if (startx_ < 0) startx_ = 0;

    win_ = newwin(height_, width_, starty_, startx_);
}

void ConfirmDialog::destroy() {
    if (win_) {
        delwin(win_);
        win_ = nullptr;
    }
}

void ConfirmDialog::render() {
    if (!win_) return;

    werase(win_);

    box(win_, 0, 0);

    if (has_colors()) {
        wattron(win_, COLOR_PAIR(1) | A_BOLD);
    }
    mvwprintw(win_, 0, 2, " Confirm Database Update ");
    if (has_colors()) {
        wattroff(win_, COLOR_PAIR(1) | A_BOLD);
    }

    mvwprintw(win_, 2, 3, "Execute the following SQL update?");

    int maxSqlLen = width_ - 6;
    std::string sqlDisp = sqlStatement_;
    if (static_cast<int>(sqlDisp.length()) > maxSqlLen) {
        sqlDisp = sqlDisp.substr(0, maxSqlLen - 3) + "...";
    }

    wattron(win_, A_BOLD);
    mvwprintw(win_, 3, 3, "%s", sqlDisp.c_str());
    wattroff(win_, A_BOLD);

    mvwprintw(win_, height_ - 2, 3, "Press [Enter] Execute UPDATE | [Esc] Cancel");

    wrefresh(win_);
}

void ConfirmDialog::handleInput(int ch, bool& confirmed, bool& cancelled) {
    confirmed = false;
    cancelled = false;

    if (ch == 27) { // Esc
        cancelled = true;
        return;
    }

    if (ch == '\n' || ch == KEY_ENTER) {
        confirmed = true;
        return;
    }
}

} // namespace dbterm
