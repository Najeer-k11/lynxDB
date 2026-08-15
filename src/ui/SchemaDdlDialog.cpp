#include "ui/SchemaDdlDialog.h"
#include <cctype>
#include <algorithm>

namespace dbterm {

SchemaDdlDialog::~SchemaDdlDialog() {
    destroy();
}

void SchemaDdlDialog::init(int termHeight, int termWidth, DdlAction action, const std::string& targetTable, const std::string& targetCol) {
    destroy();
    action_ = action;
    targetTable_ = targetTable;
    targetCol_ = targetCol;
    activeField_ = 0;

    switch (action_) {
        case DdlAction::CREATE_TABLE:
            field1_ = "new_table";
            field2_ = "id INT PRIMARY KEY, name VARCHAR(255)";
            break;
        case DdlAction::DROP_TABLE:
            field1_ = targetTable_;
            field2_ = "";
            break;
        case DdlAction::ADD_COLUMN:
            field1_ = targetTable_;
            field2_ = "new_column VARCHAR(255)";
            break;
        case DdlAction::RENAME_COLUMN:
            field1_ = targetCol_;
            field2_ = "renamed_column";
            break;
        case DdlAction::DROP_COLUMN:
            field1_ = targetCol_;
            field2_ = "";
            break;
        case DdlAction::CREATE_INDEX:
            field1_ = "idx_" + targetTable_ + "_" + (targetCol_.empty() ? "col" : targetCol_);
            field2_ = targetCol_.empty() ? "column_name" : targetCol_;
            break;
        case DdlAction::DROP_INDEX:
            field1_ = "idx_" + targetTable_ + "_" + targetCol_;
            field2_ = "";
            break;
        case DdlAction::CREATE_VIEW:
            field1_ = "v_" + targetTable_;
            field2_ = "SELECT * FROM `" + targetTable_ + "`";
            break;
        case DdlAction::DROP_VIEW:
            field1_ = targetTable_;
            field2_ = "";
            break;
    }

    width_ = std::min(74, std::max(54, termWidth - 8));
    height_ = (action_ == DdlAction::DROP_TABLE || action_ == DdlAction::DROP_COLUMN || action_ == DdlAction::DROP_VIEW || action_ == DdlAction::DROP_INDEX) ? 7 : 9;
    starty_ = (termHeight - height_) / 2;
    if (starty_ < 0) starty_ = 0;
    startx_ = (termWidth - width_) / 2;
    if (startx_ < 0) startx_ = 0;

    win_ = newwin(height_, width_, starty_, startx_);
}

void SchemaDdlDialog::destroy() {
    if (win_) {
        delwin(win_);
        win_ = nullptr;
    }
}

std::string SchemaDdlDialog::generateSql(bool isSqlite) const {
    std::string quote = isSqlite ? "\"" : "`";
    switch (action_) {
        case DdlAction::CREATE_TABLE:
            return "CREATE TABLE " + quote + field1_ + quote + " (" + field2_ + ");";
        case DdlAction::DROP_TABLE:
            return "DROP TABLE " + quote + field1_ + quote + ";";
        case DdlAction::ADD_COLUMN:
            return "ALTER TABLE " + quote + targetTable_ + quote + " ADD COLUMN " + field2_ + ";";
        case DdlAction::RENAME_COLUMN:
            return "ALTER TABLE " + quote + targetTable_ + quote + " RENAME COLUMN " + quote + field1_ + quote + " TO " + quote + field2_ + quote + ";";
        case DdlAction::DROP_COLUMN:
            return "ALTER TABLE " + quote + targetTable_ + quote + " DROP COLUMN " + quote + field1_ + quote + ";";
        case DdlAction::CREATE_INDEX:
            return "CREATE INDEX " + quote + field1_ + quote + " ON " + quote + targetTable_ + quote + " (" + field2_ + ");";
        case DdlAction::DROP_INDEX:
            return isSqlite ? ("DROP INDEX " + quote + field1_ + quote + ";") : ("DROP INDEX " + quote + field1_ + quote + " ON " + quote + targetTable_ + quote + ";");
        case DdlAction::CREATE_VIEW:
            return "CREATE VIEW " + quote + field1_ + quote + " AS " + field2_ + ";";
        case DdlAction::DROP_VIEW:
            return "DROP VIEW " + quote + field1_ + quote + ";";
    }
    return "";
}

void SchemaDdlDialog::render() {
    if (!win_) return;

    werase(win_);
    box(win_, 0, 0);

    std::string title = " Schema DDL: ";
    switch (action_) {
        case DdlAction::CREATE_TABLE: title += "Create Table"; break;
        case DdlAction::DROP_TABLE: title += "Drop Table"; break;
        case DdlAction::ADD_COLUMN: title += "Add Column to " + targetTable_; break;
        case DdlAction::RENAME_COLUMN: title += "Rename Column in " + targetTable_; break;
        case DdlAction::DROP_COLUMN: title += "Drop Column from " + targetTable_; break;
        case DdlAction::CREATE_INDEX: title += "Create Index on " + targetTable_; break;
        case DdlAction::DROP_INDEX: title += "Drop Index"; break;
        case DdlAction::CREATE_VIEW: title += "Create View"; break;
        case DdlAction::DROP_VIEW: title += "Drop View"; break;
    }

    if (has_colors()) {
        wattron(win_, COLOR_PAIR(1) | A_BOLD);
    }
    mvwprintw(win_, 0, 2, "%s", title.c_str());
    if (has_colors()) {
        wattroff(win_, COLOR_PAIR(1) | A_BOLD);
    }

    std::string label1 = "Name / Target:";
    if (action_ == DdlAction::CREATE_TABLE) label1 = "Table Name:";
    else if (action_ == DdlAction::ADD_COLUMN) label1 = "Table:";
    else if (action_ == DdlAction::RENAME_COLUMN) label1 = "Old Column:";
    else if (action_ == DdlAction::CREATE_INDEX) label1 = "Index Name:";
    else if (action_ == DdlAction::CREATE_VIEW) label1 = "View Name:";

    mvwprintw(win_, 2, 3, "%-14s", label1.c_str());

    int fieldWidth = width_ - 22;
    std::string disp1 = field1_;
    if (static_cast<int>(disp1.length()) > fieldWidth) disp1 = disp1.substr(disp1.length() - fieldWidth);

    if (activeField_ == 0) {
        if (has_colors()) wattron(win_, COLOR_PAIR(2) | A_BOLD); else wattron(win_, A_REVERSE);
    }
    mvwprintw(win_, 2, 18, "[ %-*s ]", fieldWidth, disp1.c_str());
    if (activeField_ == 0) {
        if (has_colors()) wattroff(win_, COLOR_PAIR(2) | A_BOLD); else wattroff(win_, A_REVERSE);
    }

    if (height_ >= 9) {
        std::string label2 = "Definition:";
        if (action_ == DdlAction::ADD_COLUMN) label2 = "Col Definition:";
        else if (action_ == DdlAction::RENAME_COLUMN) label2 = "New Column:";
        else if (action_ == DdlAction::CREATE_INDEX) label2 = "Columns:";
        else if (action_ == DdlAction::CREATE_VIEW) label2 = "AS Query:";

        mvwprintw(win_, 4, 3, "%-14s", label2.c_str());

        std::string disp2 = field2_;
        if (static_cast<int>(disp2.length()) > fieldWidth) disp2 = disp2.substr(disp2.length() - fieldWidth);

        if (activeField_ == 1) {
            if (has_colors()) wattron(win_, COLOR_PAIR(2) | A_BOLD); else wattron(win_, A_REVERSE);
        }
        mvwprintw(win_, 4, 18, "[ %-*s ]", fieldWidth, disp2.c_str());
        if (activeField_ == 1) {
            if (has_colors()) wattroff(win_, COLOR_PAIR(2) | A_BOLD); else wattroff(win_, A_REVERSE);
        }
    }

    mvwprintw(win_, height_ - 2, 3, "Press [Tab] Switch Field | [Enter/F5] Confirm DDL | [Esc] Cancel");

    wrefresh(win_);
}

void SchemaDdlDialog::handleInput(int ch, bool& submitted, bool& cancelled) {
    submitted = false;
    cancelled = false;

    if (ch == 27) { // Esc
        cancelled = true;
        return;
    }

    if (ch == KEY_F(5)) {
        submitted = true;
        return;
    }

    if (ch == '\t' || ch == KEY_DOWN || ch == KEY_UP) {
        if (height_ >= 9) {
            activeField_ = 1 - activeField_;
        }
        return;
    }

    if (ch == '\n' || ch == KEY_ENTER) {
        if (height_ >= 9 && activeField_ == 0) {
            activeField_ = 1;
        } else {
            submitted = true;
        }
        return;
    }

    std::string& curBuf = (activeField_ == 0) ? field1_ : field2_;
    if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
        if (!curBuf.empty()) curBuf.pop_back();
    } else if (std::isprint(ch)) {
        if (curBuf.length() < 256) curBuf.push_back(static_cast<char>(ch));
    }
}

} // namespace dbterm
