#include "ui/ConnectionDialog.h"
#include <cctype>
#include <algorithm>

namespace dbterm {

ConnectionDialog::~ConnectionDialog() {
    destroy();
}

void ConnectionDialog::init(int termHeight, int termWidth) {
    destroy();
    height_ = 20;
    width_ = 62;
    starty_ = (termHeight - height_) / 2;
    if (starty_ < 0) starty_ = 0;
    startx_ = (termWidth - width_) / 2;
    if (startx_ < 0) startx_ = 0;

    win_ = newwin(height_, width_, starty_, startx_);
}

void ConnectionDialog::destroy() {
    if (win_) {
        delwin(win_);
        win_ = nullptr;
    }
}

ConnectionConfig ConnectionDialog::getConfig() const {
    if (modeIdx_ == 3) {
        std::string err;
        return ConnectionConfig::parseURI(uriInput_, err);
    }

    ConnectionConfig cfg;
    if (modeIdx_ == 2) {
        cfg.type = DatabaseType::SQLITE;
        cfg.category = ConnectionCategory::SQLITE_FILE;
        cfg.host = host_;
        cfg.name = "sqlite: " + host_;
    } else {
        cfg.type = DatabaseType::MYSQL;
        cfg.category = (modeIdx_ == 0) ? ConnectionCategory::LOCAL_MYSQL : ConnectionCategory::ONLINE_MYSQL;
        cfg.host = host_.empty() ? "127.0.0.1" : host_;
        try {
            cfg.port = std::stoi(port_);
        } catch (...) {
            cfg.port = 3306;
        }
        cfg.user = user_.empty() ? "root" : user_;
        cfg.password = password_;
        cfg.database = database_;
        cfg.name = (modeIdx_ == 0) ? "Localhost (" + user_ + ")" : "Online (" + host_ + ")";
    }

    return cfg;
}

void ConnectionDialog::setConfig(const ConnectionConfig& config) {
    if (config.type == DatabaseType::SQLITE) {
        modeIdx_ = 2;
        host_ = config.host;
    } else if (config.category == ConnectionCategory::ONLINE_MYSQL) {
        modeIdx_ = 1;
        host_ = config.host;
        port_ = std::to_string(config.port);
        user_ = config.user;
        password_ = config.password;
        database_ = config.database;
    } else {
        modeIdx_ = 0;
        host_ = config.host;
        port_ = std::to_string(config.port);
        user_ = config.user;
        password_ = config.password;
        database_ = config.database;
    }
}

void ConnectionDialog::render(bool active) {
    if (!win_) return;

    werase(win_);

    if (active && has_colors()) {
        wattron(win_, COLOR_PAIR(1) | A_BOLD);
    }

    box(win_, 0, 0);
    mvwprintw(win_, 0, 2, " Database Connection Setup ");

    if (active && has_colors()) {
        wattroff(win_, COLOR_PAIR(1) | A_BOLD);
    }

    struct FieldDef {
        const char* label;
        const std::string* val;
        bool isPassword;
    };

    std::string modeStr = modes_[modeIdx_] + " (←/→ switch)";

    FieldDef fields[] = {
        {"Preset Mode:",&modeStr,  false},
        {"Host / Path:",&host_,     false},
        {"Port:",       &port_,     false},
        {"Username:",   &user_,     false},
        {"Password:",   &password_, true },
        {"Database:",   &database_, false},
        {"Paste URI:",  &uriInput_, false}
    };

    int numFields = (modeIdx_ == 3) ? 7 : 6;
    for (int i = 0; i < numFields; ++i) {
        bool isFocused = (active && i == activeField_);
        int lineY = 2 + i * 2;

        mvwprintw(win_, lineY, 3, "%-12s", fields[i].label);

        std::string displayVal;
        if (fields[i].isPassword) {
            displayVal = std::string(fields[i].val->length(), '*');
        } else {
            displayVal = *fields[i].val;
        }

        int fieldWidth = width_ - 20;
        if (static_cast<int>(displayVal.length()) > fieldWidth) {
            displayVal = displayVal.substr(displayVal.length() - fieldWidth);
        }

        if (isFocused) {
            if (has_colors()) {
                wattron(win_, COLOR_PAIR(2) | A_BOLD);
            } else {
                wattron(win_, A_REVERSE);
            }
        }

        mvwprintw(win_, lineY, 16, "[ %-*s ]", fieldWidth, displayVal.c_str());

        if (isFocused) {
            if (has_colors()) {
                wattroff(win_, COLOR_PAIR(2) | A_BOLD);
            } else {
                wattroff(win_, A_REVERSE);
            }
        }
    }

    mvwprintw(win_, height_ - 2, 3, "Press [Enter] Connect | [Esc] Cancel | [Tab] Next");

    wrefresh(win_);
}

void ConnectionDialog::handleInput(int ch, bool& submitted, bool& cancelled) {
    submitted = false;
    cancelled = false;

    if (ch == 27) {
        cancelled = true;
        return;
    }

    if (ch == '\n' || ch == KEY_ENTER) {
        submitted = true;
        return;
    }

    int totalFields = (modeIdx_ == 3) ? 7 : 6;

    if (ch == '\t' || ch == KEY_DOWN) {
        activeField_ = (activeField_ + 1) % totalFields;
        return;
    }

    if (ch == KEY_UP) {
        activeField_ = (activeField_ + totalFields - 1) % totalFields;
        return;
    }

    if (activeField_ == 0) { // Preset mode switcher
        if (ch == ' ' || ch == KEY_LEFT || ch == KEY_RIGHT) {
            if (ch == KEY_LEFT) {
                modeIdx_ = (modeIdx_ + 3) % 4;
            } else {
                modeIdx_ = (modeIdx_ + 1) % 4;
            }
            if (modeIdx_ == 0) {
                host_ = "127.0.0.1"; port_ = "3306"; user_ = "root";
            } else if (modeIdx_ == 1) {
                host_ = "remote-db.example.com"; port_ = "3306"; user_ = "admin";
            } else if (modeIdx_ == 2) {
                host_ = "/home/venx/database.db";
            }
            return;
        }
    }

    std::string* activeStr = nullptr;
    switch (activeField_) {
        case 1: activeStr = &host_; break;
        case 2: activeStr = &port_; break;
        case 3: activeStr = &user_; break;
        case 4: activeStr = &password_; break;
        case 5: activeStr = &database_; break;
        case 6: activeStr = &uriInput_; break;
        default: break;
    }

    if (!activeStr || activeField_ == 0) return;

    if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
        if (!activeStr->empty()) {
            activeStr->pop_back();
        }
    } else if (std::isprint(ch) && ch != '\t') {
        if (activeStr->length() < 256) {
            activeStr->push_back(static_cast<char>(ch));
        }
    }
}

} // namespace dbterm
