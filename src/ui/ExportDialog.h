#ifndef DBTERM_EXPORTDIALOG_H
#define DBTERM_EXPORTDIALOG_H

#include <ncursesw/ncurses.h>
#include <string>
#include <vector>

namespace dbterm {

enum class ExportFormat {
    CSV,
    JSON
};

class ExportDialog {
public:
    ExportDialog() = default;
    ~ExportDialog();

    void init(int termHeight, int termWidth, const std::string& defaultTableName);
    void render();
    void destroy();

    void handleInput(int ch, bool& submitted, bool& cancelled);
    std::string getFilePath() const { return filePath_; }
    ExportFormat getFormat() const { return format_; }

    static bool exportToCsv(const std::string& path, const std::vector<std::string>& headers, const std::vector<std::vector<std::string>>& rows, std::string& err);
    static bool exportToJson(const std::string& path, const std::vector<std::string>& headers, const std::vector<std::vector<std::string>>& rows, std::string& err);

private:
    WINDOW* win_{nullptr};
    int height_{8};
    int width_{60};
    int starty_{0};
    int startx_{0};
    std::string filePath_{"export.csv"};
    ExportFormat format_{ExportFormat::CSV};
};

} // namespace dbterm

#endif // DBTERM_EXPORTDIALOG_H
