#include "ui/Sidebar.h"
#include <algorithm>

namespace dbterm {

Sidebar::~Sidebar() {
    destroy();
}

void Sidebar::init(int height, int width, int starty, int startx) {
    destroy();
    height_ = height;
    width_ = width;
    starty_ = starty;
    startx_ = startx;
    win_ = newwin(height_, width_, starty_, startx_);
}

void Sidebar::destroy() {
    if (win_) {
        delwin(win_);
        win_ = nullptr;
    }
}

void Sidebar::render(const AppState& state) {
    if (!win_) return;

    werase(win_);

    bool isActive = (state.activePanel == Panel::SIDEBAR);

    if (isActive && has_colors()) {
        wattron(win_, COLOR_PAIR(1) | A_BOLD);
    }

    box(win_, 0, 0);
    mvwprintw(win_, 0, 2, " Connections ");

    if (isActive && has_colors()) {
        wattroff(win_, COLOR_PAIR(1) | A_BOLD);
    }

    int maxItems = height_ - 2;
    int itemCount = static_cast<int>(state.visibleTreeNodes.size());

    // Calculate vertical scroll offset to keep selected tree node in view
    int scrollOffset = 0;
    if (state.sidebarSelectedIndex >= maxItems) {
        scrollOffset = state.sidebarSelectedIndex - maxItems + 1;
    }

    int visibleItems = std::min(maxItems, itemCount - scrollOffset);

    for (int i = 0; i < visibleItems; ++i) {
        int actualIdx = scrollOffset + i;
        bool isSelected = (actualIdx == state.sidebarSelectedIndex);
        const auto& item = state.visibleTreeNodes[actualIdx];

        if (isSelected) {
            if (isActive && has_colors()) {
                wattron(win_, COLOR_PAIR(2) | A_BOLD);
            } else {
                wattron(win_, A_REVERSE);
            }
        }

        std::string line = item.node->getPrefix() + item.node->name;
        if (static_cast<int>(line.length()) > width_ - 4) {
            line = line.substr(0, width_ - 4);
        }

        mvwprintw(win_, i + 1, 2, "%-*s", width_ - 4, line.c_str());

        if (isSelected) {
            if (isActive && has_colors()) {
                wattroff(win_, COLOR_PAIR(2) | A_BOLD);
            } else {
                wattroff(win_, A_REVERSE);
            }
        }
    }

    wrefresh(win_);
}

} // namespace dbterm
