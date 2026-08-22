#ifndef DBTERM_CONFIRMDIALOG_H
#define DBTERM_CONFIRMDIALOG_H

#include "ui/CursesCompat.h"
#include <string>

namespace dbterm {

class ConfirmDialog {
public:
    ConfirmDialog() = default;
    ~ConfirmDialog();

    void init(int termHeight, int termWidth, const std::string& sqlStatement);
    void render();
    void destroy();

    void handleInput(int ch, bool& confirmed, bool& cancelled);
    std::string getSql() const { return sqlStatement_; }

private:
    WINDOW* win_{nullptr};
    int height_{8};
    int width_{66};
    int starty_{0};
    int startx_{0};
    std::string sqlStatement_;
};

} // namespace dbterm

#endif // DBTERM_CONFIRMDIALOG_H
