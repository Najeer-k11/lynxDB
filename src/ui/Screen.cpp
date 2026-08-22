#include "ui/Screen.h"
#include "ui/CursesCompat.h"

namespace dbterm {

Screen::Screen() {
    termHeight_ = 0;
    termWidth_ = 0;
}

void Screen::updateDimensions(const AppState& state) {
    int curH, curW;
    getmaxyx(stdscr, curH, curW);

    if (curH != termHeight_ || curW != termWidth_) {
        termHeight_ = curH;
        termWidth_ = curW;

        int statusHeight = 1;
        int mainHeight = termHeight_ - statusHeight;
        if (mainHeight < 1) mainHeight = 1;

        int sidebarWidth = std::max(25, termWidth_ * 3 / 10);
        if (sidebarWidth > termWidth_ - 10) {
            sidebarWidth = termWidth_ / 2;
        }
        int contentWidth = termWidth_ - sidebarWidth;

        sidebar_.init(mainHeight, sidebarWidth, 0, 0);
        tableView_.init(mainHeight, contentWidth, 0, sidebarWidth);
        welcomeView_.init(mainHeight, contentWidth, 0, sidebarWidth);
        erDiagramView_.init(mainHeight, contentWidth, 0, sidebarWidth);
        statusBar_.init(statusHeight, termWidth_, mainHeight, 0);

        connectionDialog_.init(termHeight_, termWidth_);
        errorDialog_.init(termHeight_, termWidth_, "Database Error", state.errorMessage);
        sqlQueryDialog_.init(termHeight_, termWidth_);
        cellEditDialog_.init(termHeight_, termWidth_, state.editTargetColumn, "");
        confirmDialog_.init(termHeight_, termWidth_, state.pendingUpdateSql);
        filterDialog_.init(termHeight_, termWidth_);
        exportDialog_.init(termHeight_, termWidth_, state.activeTableName);
        rowInsertDialog_.init(termHeight_, termWidth_, state.contentHeaders);
        schemaDdlDialog_.init(termHeight_, termWidth_, DdlAction::CREATE_TABLE);
        goToRowDialog_.init(termHeight_, termWidth_, 100);

        clear();
        refresh();
    }
}

void Screen::render(const AppState& state) {
    updateDimensions(state);

    sidebar_.render(state);

    if (!state.isConnected && state.viewMode == ViewMode::WELCOME) {
        welcomeView_.render(state);
    } else if (state.viewMode == ViewMode::ER_DIAGRAM) {
        erDiagramView_.render();
    } else {
        tableView_.render(state);
    }

    statusBar_.render(state);

    if (state.activeDialog == DialogType::CONNECTION) {
        connectionDialog_.init(termHeight_, termWidth_);
        connectionDialog_.render(true);
    } else if (state.activeDialog == DialogType::ERROR_POPUP) {
        errorDialog_.init(termHeight_, termWidth_, "Database Error", state.errorMessage);
        errorDialog_.render();
    } else if (state.activeDialog == DialogType::SQL_PROMPT) {
        sqlQueryDialog_.init(termHeight_, termWidth_);
        sqlQueryDialog_.render();
    } else if (state.activeDialog == DialogType::CELL_EDIT) {
        cellEditDialog_.render();
    } else if (state.activeDialog == DialogType::CONFIRM_UPDATE) {
        confirmDialog_.render();
    } else if (state.activeDialog == DialogType::FILTER_PROMPT) {
        filterDialog_.init(termHeight_, termWidth_);
        filterDialog_.render();
    } else if (state.activeDialog == DialogType::EXPORT_PROMPT) {
        exportDialog_.render();
    } else if (state.activeDialog == DialogType::ROW_INSERT) {
        rowInsertDialog_.render();
    } else if (state.activeDialog == DialogType::SCHEMA_DDL) {
        schemaDdlDialog_.render();
    } else if (state.activeDialog == DialogType::GO_TO_ROW) {
        goToRowDialog_.render();
    }
}

} // namespace dbterm
