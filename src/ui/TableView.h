#ifndef DBTERM_TABLEVIEW_H
#define DBTERM_TABLEVIEW_H

#include "app/App.h"
#include <ncursesw/ncurses.h>

namespace dbterm {

class TableView {
public:
    TableView() = default;
    ~TableView();

    void init(int height, int width, int starty, int startx);
    void render(const AppState& state);
    void destroy();

private:
    WINDOW* win_{nullptr};
    int height_{0};
    int width_{0};
    int starty_{0};
    int startx_{0};
};

} // namespace dbterm

#endif // DBTERM_TABLEVIEW_H
