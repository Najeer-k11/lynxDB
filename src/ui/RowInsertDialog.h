#ifndef DBTERM_ROWINSERTDIALOG_H
#define DBTERM_ROWINSERTDIALOG_H

#include "ui/CursesCompat.h"
#include <string>
#include <vector>

namespace dbterm {

class RowInsertDialog {
public:
    RowInsertDialog() = default;
    ~RowInsertDialog();

    void init(int termHeight, int termWidth, const std::vector<std::string>& headers);
    void render();
    void destroy();

    void handleInput(int ch, bool& submitted, bool& cancelled);
    std::vector<std::string> getValues() const { return values_; }

private:
    WINDOW* win_{nullptr};
    int height_{12};
    int width_{64};
    int starty_{0};
    int startx_{0};
    std::vector<std::string> headers_;
    std::vector<std::string> values_;
    int activeFieldIndex_{0};
};

} // namespace dbterm

#endif // DBTERM_ROWINSERTDIALOG_H
