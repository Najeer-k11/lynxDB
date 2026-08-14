#include "ui/WelcomeView.h"
#include "models/ConfigManager.h"
#include "models/ThemeManager.h"
#include <algorithm>

namespace dbterm {

WelcomeView::~WelcomeView() {
    destroy();
}

void WelcomeView::init(int height, int width, int starty, int startx) {
    destroy();
    height_ = height;
    width_ = width;
    starty_ = starty;
    startx_ = startx;
    win_ = newwin(height_, width_, starty_, startx_);
}

void WelcomeView::destroy() {
    if (win_) {
        delwin(win_);
        win_ = nullptr;
    }
}

void WelcomeView::render(const AppState& state) {
    if (!win_) return;

    werase(win_);

    if (has_colors()) {
        wattron(win_, COLOR_PAIR(1) | A_BOLD);
    }
    box(win_, 0, 0);
    mvwprintw(win_, 0, 2, " Welcome to lynxDB ");
    if (has_colors()) {
        wattroff(win_, COLOR_PAIR(1) | A_BOLD);
    }

    int centerY = 2;
    int centerX = (width_ - 36) / 2;
    if (centerX < 2) centerX = 2;

    if (has_colors()) {
        wattron(win_, COLOR_PAIR(1) | A_BOLD);
    }
    mvwprintw(win_, centerY,     centerX, "  _                 ____  ____  ");
    mvwprintw(win_, centerY + 1, centerX, " | |_   _ _ __  _  _|  _ \\| __ ) ");
    mvwprintw(win_, centerY + 2, centerX, " | | | | | '_ \\ \\/ /| | | |  _ \\ ");
    mvwprintw(win_, centerY + 3, centerX, " | | |_| | | | >  < | |_| | |_) |");
    mvwprintw(win_, centerY + 4, centerX, " |_|\\__,_|_| |_/_/\\_\\____/|____/ ");
    mvwprintw(win_, centerY + 5, centerX, "    |___/                       ");
    if (has_colors()) {
        wattroff(win_, COLOR_PAIR(1) | A_BOLD);
    }

    wattron(win_, A_BOLD);
    mvwprintw(win_, centerY + 7, std::max(2, (width_ - 48) / 2), "Fast, Keyboard-Driven Terminal Database Explorer");
    wattroff(win_, A_BOLD);

    int savedY = centerY + 9;
    int savedX = std::max(3, (width_ - 56) / 2);

    auto savedConfigs = ConfigManager::loadConnections();
    if (!savedConfigs.empty()) {
        if (has_colors()) {
            wattron(win_, COLOR_PAIR(1) | A_BOLD);
        } else {
            wattron(win_, A_BOLD);
        }
        mvwprintw(win_, savedY, savedX, "Saved Connections (Press Enter to Connect, 'x' to Remove):");
        if (has_colors()) {
            wattroff(win_, COLOR_PAIR(1) | A_BOLD);
        } else {
            wattroff(win_, A_BOLD);
        }

        int maxVisSaved = std::min(5, static_cast<int>(savedConfigs.size()));
        for (int i = 0; i < maxVisSaved; ++i) {
            bool isSelected = (i == state.welcomeSelectedIndex);
            const auto& cfg = savedConfigs[i];

            std::string label = "[" + std::to_string(i + 1) + "] " + cfg.name + " (" + cfg.host + ")";

            if (isSelected) {
                if (has_colors()) {
                    wattron(win_, COLOR_PAIR(2) | A_BOLD);
                } else {
                    wattron(win_, A_REVERSE);
                }
                mvwprintw(win_, savedY + 1 + i, savedX + 2, "▸ %-*s", width_ - savedX - 8, label.c_str());
                if (has_colors()) {
                    wattroff(win_, COLOR_PAIR(2) | A_BOLD);
                } else {
                    wattroff(win_, A_REVERSE);
                }
            } else {
                mvwprintw(win_, savedY + 1 + i, savedX + 2, "  %-*s", width_ - savedX - 8, label.c_str());
            }
        }
        savedY += maxVisSaved + 2;
    } else {
        savedY += 1;
    }

    if (has_colors()) {
        wattron(win_, COLOR_PAIR(1) | A_BOLD);
    } else {
        wattron(win_, A_BOLD);
    }
    mvwprintw(win_, savedY, savedX, "Quick Actions:");
    if (has_colors()) {
        wattroff(win_, COLOR_PAIR(1) | A_BOLD);
    } else {
        wattroff(win_, A_BOLD);
    }

    const auto& themes = ThemeManager::getThemes();
    std::string curTheme = themes[state.currentThemeIndex % themes.size()].name;

    mvwprintw(win_, savedY + 1, savedX + 2, "• Press [ c ] or [ n ]  New Connection (MySQL / SQLite / URI)");
    mvwprintw(win_, savedY + 2, savedX + 2, "• Press [ a ] or [ F2 ] Cycle Neon Accent Theme (Active: %s)", curTheme.c_str());
    mvwprintw(win_, savedY + 3, savedX + 2, "• Press [ x ] or [ Del ] Remove Highlighted Saved Connection");
    mvwprintw(win_, savedY + 4, savedX + 2, "• Press [ Tab ]        Switch Focus between Sidebar & Main Area");
    mvwprintw(win_, savedY + 5, savedX + 2, "• Press [ : ]          Execute Custom SQL Statement");
    mvwprintw(win_, savedY + 6, savedX + 2, "• Press [ q ]          Quit lynxDB");

    int footerY = height_ - 2;
    mvwprintw(win_, footerY, std::max(2, (width_ - 54) / 2), "Author: Venkata Najeer Kopparapu (github.com/Najeer-k11)");

    wrefresh(win_);
}

} // namespace dbterm
