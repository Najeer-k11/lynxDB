#ifndef DBTERM_SCHEMADDLDIALOG_H
#define DBTERM_SCHEMADDLDIALOG_H

#include "ui/CursesCompat.h"
#include <string>
#include <vector>

namespace dbterm {

enum class DdlAction {
    CREATE_TABLE,
    DROP_TABLE,
    ADD_COLUMN,
    RENAME_COLUMN,
    DROP_COLUMN,
    CREATE_INDEX,
    DROP_INDEX,
    CREATE_VIEW,
    DROP_VIEW
};

class SchemaDdlDialog {
public:
    SchemaDdlDialog() = default;
    ~SchemaDdlDialog();

    void init(int termHeight, int termWidth, DdlAction action, const std::string& targetTable = "", const std::string& targetCol = "");
    void render();
    void destroy();

    void handleInput(int ch, bool& submitted, bool& cancelled);

    std::string generateSql(bool isSqlite) const;
    DdlAction getAction() const { return action_; }

private:
    WINDOW* win_{nullptr};
    int height_{10};
    int width_{64};
    int starty_{0};
    int startx_{0};

    DdlAction action_{DdlAction::CREATE_TABLE};
    std::string targetTable_;
    std::string targetCol_;

    std::string field1_{"new_table"};
    std::string field2_{"id INT PRIMARY KEY AUTO_INCREMENT, name VARCHAR(255)"};
    int activeField_{0};
};

} // namespace dbterm

#endif // DBTERM_SCHEMADDLDIALOG_H
