#ifndef DBTERM_THEMEMANAGER_H
#define DBTERM_THEMEMANAGER_H

#include "ui/CursesCompat.h"
#include <string>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <sys/stat.h>

namespace dbterm {

struct NeonTheme {
    std::string name;
    short fgColor;
    short textOnAccent; // High-contrast text color on accent background
};

class ThemeManager {
public:
    static const std::vector<NeonTheme>& getThemes() {
        static const std::vector<NeonTheme> themes = {
            {"Neon Cyan", COLOR_CYAN, COLOR_WHITE},
            {"Neon Green", COLOR_GREEN, COLOR_WHITE},
            {"Neon Magenta", COLOR_MAGENTA, COLOR_WHITE},
            {"Neon Yellow", COLOR_YELLOW, COLOR_WHITE},
            {"Neon Red", COLOR_RED, COLOR_WHITE},
            {"Neon Blue", COLOR_BLUE, COLOR_WHITE}
        };
        return themes;
    }

    static std::string getThemeFilePath() {
        const char* home = std::getenv("HOME");
        std::string base = home ? home : ".";
        std::string dir = base + "/.config/lynxdb";
        mkdir(dir.c_str(), 0755);
        return dir + "/theme.cfg";
    }

    static int loadSavedThemeIndex() {
        std::ifstream in(getThemeFilePath());
        if (!in.is_open()) return 0;
        int idx = 0;
        if (in >> idx) {
            if (idx >= 0 && idx < static_cast<int>(getThemes().size())) {
                return idx;
            }
        }
        return 0;
    }

    static void saveThemeIndex(int index) {
        std::ofstream out(getThemeFilePath());
        if (out.is_open()) {
            out << index << "\n";
        }
    }

    static void applyTheme(int themeIndex) {
        if (!has_colors()) return;

        const auto& themes = getThemes();
        int safeIdx = (themeIndex % static_cast<int>(themes.size()) + static_cast<int>(themes.size())) % static_cast<int>(themes.size());
        short fg = themes[safeIdx].fgColor;
        short textCol = themes[safeIdx].textOnAccent;

        // Pair 1: Primary Neon Accent (Borders, Headers, Titles, ASCII Art)
        init_pair(1, fg, -1);

        // Pair 2: Selected Item / Highlighted Cell (High Contrast Text on Neon Accent)
        init_pair(2, textCol, fg);

        // Pair 3: Status Bar (High Contrast Text on Neon Accent)
        init_pair(3, textCol, fg);

        // Pair 4: Header / Accent Text
        init_pair(4, fg, -1);
    }
};

} // namespace dbterm

#endif // DBTERM_THEMEMANAGER_H
