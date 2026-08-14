#ifndef DBTERM_FILTERDIALOG_H
#define DBTERM_FILTERDIALOG_H

#include <ncursesw/ncurses.h>
#include <string>

namespace dbterm {

class FilterDialog {
public:
    FilterDialog() = default;
    ~FilterDialog();

    void init(int termHeight, int termWidth);
    void render();
    void destroy();

    void handleInput(int ch, bool& submitted, bool& cancelled);
    std::string getFilterQuery() const { return filterInput_; }
    void setFilterQuery(const std::string& query) { filterInput_ = query; }

private:
    WINDOW* win_{nullptr};
    int height_{5};
    int width_{56};
    int starty_{0};
    int startx_{0};
    std::string filterInput_;
};

} // namespace dbterm

#endif // DBTERM_FILTERDIALOG_H
