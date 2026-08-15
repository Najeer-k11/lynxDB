#ifndef DBTERM_SCREEN_H
#define DBTERM_SCREEN_H

#include "app/App.h"
#include "ui/Sidebar.h"
#include "ui/TableView.h"
#include "ui/WelcomeView.h"
#include "ui/StatusBar.h"
#include "ui/ConnectionDialog.h"
#include "ui/ErrorDialog.h"
#include "ui/SqlQueryDialog.h"
#include "ui/CellEditDialog.h"
#include "ui/ConfirmDialog.h"
#include "ui/FilterDialog.h"
#include "ui/ExportDialog.h"
#include "ui/RowInsertDialog.h"
#include "ui/SchemaDdlDialog.h"
#include "ui/GoToRowDialog.h"
#include "ui/ErDiagramView.h"

namespace dbterm {

class Screen {
public:
    Screen();
    ~Screen() = default;

    void render(const AppState& state);

    ConnectionDialog& connectionDialog() { return connectionDialog_; }
    ErrorDialog& errorDialog() { return errorDialog_; }
    SqlQueryDialog& sqlQueryDialog() { return sqlQueryDialog_; }
    CellEditDialog& cellEditDialog() { return cellEditDialog_; }
    ConfirmDialog& confirmDialog() { return confirmDialog_; }
    FilterDialog& filterDialog() { return filterDialog_; }
    ExportDialog& exportDialog() { return exportDialog_; }
    RowInsertDialog& rowInsertDialog() { return rowInsertDialog_; }
    SchemaDdlDialog& schemaDdlDialog() { return schemaDdlDialog_; }
    GoToRowDialog& goToRowDialog() { return goToRowDialog_; }
    ErDiagramView& erDiagramView() { return erDiagramView_; }

private:
    void updateDimensions(const AppState& state);

    int termHeight_{0};
    int termWidth_{0};

    Sidebar sidebar_;
    TableView tableView_;
    WelcomeView welcomeView_;
    StatusBar statusBar_;
    ConnectionDialog connectionDialog_;
    ErrorDialog errorDialog_;
    SqlQueryDialog sqlQueryDialog_;
    CellEditDialog cellEditDialog_;
    ConfirmDialog confirmDialog_;
    FilterDialog filterDialog_;
    ExportDialog exportDialog_;
    RowInsertDialog rowInsertDialog_;
    SchemaDdlDialog schemaDdlDialog_;
    GoToRowDialog goToRowDialog_;
    ErDiagramView erDiagramView_;
};

} // namespace dbterm

#endif // DBTERM_SCREEN_H
