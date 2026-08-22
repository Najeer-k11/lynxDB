#ifndef DBTERM_CELLEDITDIALOG_H
#define DBTERM_CELLEDITDIALOG_H

#include "ui/CursesCompat.h"
#include <string>

namespace dbterm {

class CellEditDialog {
public:
    CellEditDialog() = default;
    ~CellEditDialog();

    void init(int termHeight, int termWidth, const std::string& columnName, const std::string& currentValue);
    void render();
    void destroy();

    void handleInput(int ch, bool& submitted, bool& cancelled);
    std::string getNewValue() const { return editBuffer_; }

private:
    WINDOW* win_{nullptr};
    int height_{8};
    int width_{54};
    int starty_{0};
    int startx_{0};
    std::string columnName_;
    std::string editBuffer_;
};

} // namespace dbterm

#endif // DBTERM_CELLEDITDIALOG_H
