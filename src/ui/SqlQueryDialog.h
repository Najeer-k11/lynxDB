#ifndef DBTERM_SQLQUERYDIALOG_H
#define DBTERM_SQLQUERYDIALOG_H

#include <ncursesw/ncurses.h>
#include <string>

namespace dbterm {

class SqlQueryDialog {
public:
    SqlQueryDialog() = default;
    ~SqlQueryDialog();

    void init(int termHeight, int termWidth);
    void render();
    void destroy();

    void handleInput(int ch, bool& submitted, bool& cancelled);
    std::string getQuery() const { return queryInput_; }
    void setQuery(const std::string& query) { queryInput_ = query; }

private:
    WINDOW* win_{nullptr};
    int height_{10};
    int width_{66};
    int starty_{0};
    int startx_{0};
    std::string queryInput_{"SELECT * FROM users;"};
};

} // namespace dbterm

#endif // DBTERM_SQLQUERYDIALOG_H
