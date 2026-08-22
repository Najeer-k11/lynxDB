#ifndef DBTERM_SIDEBAR_H
#define DBTERM_SIDEBAR_H

#include "app/App.h"
#include "ui/CursesCompat.h"

namespace dbterm {

class Sidebar {
public:
    Sidebar() = default;
    ~Sidebar();

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

#endif // DBTERM_SIDEBAR_H
