#ifndef DBTERM_ERDIAGRAMVIEW_H
#define DBTERM_ERDIAGRAMVIEW_H

#include "database/DatabaseConnection.h"
#include <ncursesw/ncurses.h>
#include <string>
#include <vector>

namespace dbterm {

struct ErTableNode {
    std::string tableName;
    std::vector<std::pair<std::string, std::string>> columns; // colName, colType
    std::vector<std::string> primaryKeys;
    std::vector<ForeignKeyInfo> foreignKeys;
};

class ErDiagramView {
public:
    ErDiagramView() = default;
    ~ErDiagramView();

    void init(int height, int width, int starty, int startx);
    void loadSchema(DatabaseConnection* conn, const std::string& dbName);
    void render();
    void destroy();

    void handleInput(int ch, bool& closed);

private:
    WINDOW* win_{nullptr};
    int height_{0};
    int width_{0};
    int starty_{0};
    int startx_{0};

    std::string dbName_;
    std::vector<ErTableNode> tables_;
    int scrollY_{0};
    int scrollX_{0};
};

} // namespace dbterm

#endif // DBTERM_ERDIAGRAMVIEW_H
