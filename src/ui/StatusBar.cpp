#include "ui/StatusBar.h"
#include "models/ThemeManager.h"

namespace dbterm {

StatusBar::~StatusBar() {
    destroy();
}

void StatusBar::init(int height, int width, int starty, int startx) {
    destroy();
    height_ = height;
    width_ = width;
    starty_ = starty;
    startx_ = startx;
    win_ = newwin(height_, width_, starty_, startx_);
}

void StatusBar::destroy() {
    if (win_) {
        delwin(win_);
        win_ = nullptr;
    }
}

void StatusBar::render(const AppState& state) {
    if (!win_) return;

    werase(win_);

    if (has_colors()) {
        wbkgd(win_, COLOR_PAIR(3));
    } else {
        wattron(win_, A_REVERSE);
    }

    const auto& themes = ThemeManager::getThemes();
    std::string themeName = themes[state.currentThemeIndex % themes.size()].name;

    std::string shortcuts = " Tab: Switch | ↑↓/←→: Nav | /: Filter | f: FK Jump | g: GoTo | V: ER View | e: Edit | d: Del | i: Ins | E: Exp | o: Sort | a: " + themeName + " | q: Quit ";
    if (!state.contentHeaders.empty()) {
        int totalCols = static_cast<int>(state.contentHeaders.size());
        shortcuts += "| Cols " + std::to_string(state.contentSelectedColIndex + 1) + " of " + std::to_string(totalCols) + " ";
    }

    std::string message = state.statusMessage;

    mvwprintw(win_, 0, 1, "%s", shortcuts.c_str());

    int msgX = width_ - static_cast<int>(message.length()) - 2;
    if (msgX > static_cast<int>(shortcuts.length()) + 2) {
        mvwprintw(win_, 0, msgX, "%s", message.c_str());
    }

    if (!has_colors()) {
        wattroff(win_, A_REVERSE);
    }

    wrefresh(win_);
}

} // namespace dbterm
