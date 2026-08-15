#include "ui/ExportDialog.h"
#include <fstream>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <unistd.h>

namespace dbterm {

ExportDialog::~ExportDialog() {
    destroy();
}

void ExportDialog::init(int termHeight, int termWidth, const std::string& defaultTableName) {
    destroy();
    std::string base = defaultTableName.empty() ? "export" : defaultTableName;
    filePath_ = base + (format_ == ExportFormat::CSV ? ".csv" : ".json");

    width_ = std::min(74, std::max(52, termWidth - 8));
    height_ = 9;
    starty_ = (termHeight - height_) / 2;
    if (starty_ < 0) starty_ = 0;
    startx_ = (termWidth - width_) / 2;
    if (startx_ < 0) startx_ = 0;

    win_ = newwin(height_, width_, starty_, startx_);
}

void ExportDialog::destroy() {
    if (win_) {
        delwin(win_);
        win_ = nullptr;
    }
}

void ExportDialog::render() {
    if (!win_) return;

    werase(win_);

    box(win_, 0, 0);

    if (has_colors()) {
        wattron(win_, COLOR_PAIR(1) | A_BOLD);
    }
    mvwprintw(win_, 0, 2, " Export Data ");
    if (has_colors()) {
        wattroff(win_, COLOR_PAIR(1) | A_BOLD);
    }

    mvwprintw(win_, 2, 3, "Format:     [ %s ]  (Press [Tab] to toggle CSV / JSON)", (format_ == ExportFormat::CSV ? "CSV " : "JSON"));
    mvwprintw(win_, 3, 3, "Export Filepath:");

    int fieldWidth = width_ - 8;
    std::string displayVal = filePath_;
    if (static_cast<int>(displayVal.length()) > fieldWidth) {
        displayVal = displayVal.substr(displayVal.length() - fieldWidth);
    }

    if (has_colors()) {
        wattron(win_, COLOR_PAIR(2) | A_BOLD);
    } else {
        wattron(win_, A_REVERSE);
    }
    mvwprintw(win_, 4, 3, "[ %-*s ]", fieldWidth, displayVal.c_str());
    if (has_colors()) {
        wattroff(win_, COLOR_PAIR(2) | A_BOLD);
    } else {
        wattroff(win_, A_REVERSE);
    }

    char cwd[1024];
    std::string pwd = getcwd(cwd, sizeof(cwd)) ? std::string(cwd) : ".";
    std::string fullPath = (filePath_.rfind('/', 0) == 0) ? filePath_ : (pwd + "/" + filePath_);
    if (fullPath.length() > static_cast<size_t>(width_ - 8)) {
        fullPath = "..." + fullPath.substr(fullPath.length() - (width_ - 11));
    }
    mvwprintw(win_, 5, 3, "Output: %s", fullPath.c_str());

    mvwprintw(win_, height_ - 2, 3, "Press [Enter] Export File | [Esc] Cancel");

    wrefresh(win_);
}

void ExportDialog::handleInput(int ch, bool& submitted, bool& cancelled) {
    submitted = false;
    cancelled = false;

    if (ch == 27) { // Esc
        cancelled = true;
        return;
    }

    if (ch == '\t') {
        format_ = (format_ == ExportFormat::CSV ? ExportFormat::JSON : ExportFormat::CSV);
        size_t dot = filePath_.rfind('.');
        std::string stem = (dot != std::string::npos) ? filePath_.substr(0, dot) : filePath_;
        filePath_ = stem + (format_ == ExportFormat::CSV ? ".csv" : ".json");
        return;
    }

    if (ch == '\n' || ch == KEY_ENTER) {
        submitted = true;
        return;
    }

    if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
        if (!filePath_.empty()) {
            filePath_.pop_back();
        }
    } else if (std::isprint(ch)) {
        if (filePath_.length() < 256) {
            filePath_.push_back(static_cast<char>(ch));
        }
    }
}

bool ExportDialog::exportToCsv(const std::string& path, const std::vector<std::string>& headers, const std::vector<std::vector<std::string>>& rows, std::string& err) {
    std::ofstream out(path);
    if (!out.is_open()) {
        err = "Could not open file for writing: " + path;
        return false;
    }

    for (size_t i = 0; i < headers.size(); ++i) {
        out << "\"" << headers[i] << "\"";
        if (i + 1 < headers.size()) out << ",";
    }
    out << "\n";

    for (const auto& row : rows) {
        for (size_t i = 0; i < row.size(); ++i) {
            std::string cell = row[i];
            size_t pos = 0;
            while ((pos = cell.find('"', pos)) != std::string::npos) {
                cell.replace(pos, 1, "\"\"");
                pos += 2;
            }
            out << "\"" << cell << "\"";
            if (i + 1 < row.size()) out << ",";
        }
        out << "\n";
    }

    return true;
}

bool ExportDialog::exportToJson(const std::string& path, const std::vector<std::string>& headers, const std::vector<std::vector<std::string>>& rows, std::string& err) {
    std::ofstream out(path);
    if (!out.is_open()) {
        err = "Could not open file for writing: " + path;
        return false;
    }

    out << "[\n";
    for (size_t r = 0; r < rows.size(); ++r) {
        out << "  {\n";
        const auto& row = rows[r];
        for (size_t c = 0; c < headers.size(); ++c) {
            std::string val = (c < row.size()) ? row[c] : "";
            out << "    \"" << headers[c] << "\": ";
            if (val == "NULL") {
                out << "null";
            } else {
                size_t pos = 0;
                while ((pos = val.find('"', pos)) != std::string::npos) {
                    val.replace(pos, 1, "\\\"");
                    pos += 2;
                }
                out << "\"" << val << "\"";
            }
            if (c + 1 < headers.size()) out << ",";
            out << "\n";
        }
        out << "  }";
        if (r + 1 < rows.size()) out << ",";
        out << "\n";
    }
    out << "]\n";

    return true;
}

} // namespace dbterm
