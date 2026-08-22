#ifndef DBTERM_WELCOMEVIEW_H
#define DBTERM_WELCOMEVIEW_H

#include "app/App.h"
#include "ui/CursesCompat.h"

namespace dbterm {

class WelcomeView {
public:
    WelcomeView() = default;
    ~WelcomeView();

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

#endif // DBTERM_WELCOMEVIEW_H
