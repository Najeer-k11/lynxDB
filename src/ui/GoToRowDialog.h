#ifndef DBTERM_GOTOROWDIALOG_H
#define DBTERM_GOTOROWDIALOG_H

#include "ui/CursesCompat.h"
#include <string>

namespace dbterm {

class GoToRowDialog {
public:
    GoToRowDialog() = default;
    ~GoToRowDialog();

    void init(int termHeight, int termWidth, int maxRows);
    void render();
    void destroy();

    void handleInput(int ch, bool& submitted, bool& cancelled);
    int getTargetRow() const;

private:
    WINDOW* win_{nullptr};
    int height_{7};
    int width_{48};
    int starty_{0};
    int startx_{0};
    int maxRows_{1};
    std::string inputBuffer_{"1"};
};

} // namespace dbterm

#endif // DBTERM_GOTOROWDIALOG_H
