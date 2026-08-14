#ifndef DBTERM_ERRORDIALOG_H
#define DBTERM_ERRORDIALOG_H

#include <ncursesw/ncurses.h>
#include <string>

namespace dbterm {

class ErrorDialog {
public:
    ErrorDialog() = default;
    ~ErrorDialog();

    void init(int termHeight, int termWidth, const std::string& title, const std::string& message);
    void render();
    void destroy();

    void handleInput(int ch, bool& dismissed);

private:
    WINDOW* win_{nullptr};
    int height_{8};
    int width_{60};
    int starty_{0};
    int startx_{0};
    std::string title_{"Database Error"};
    std::string message_;
};

} // namespace dbterm

#endif // DBTERM_ERRORDIALOG_H
