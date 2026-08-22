#ifndef DBTERM_CONNECTIONDIALOG_H
#define DBTERM_CONNECTIONDIALOG_H

#include "models/ConnectionConfig.h"
#include "ui/CursesCompat.h"
#include <string>
#include <vector>

namespace dbterm {

class ConnectionDialog {
public:
    ConnectionDialog() = default;
    ~ConnectionDialog();

    void init(int termHeight, int termWidth);
    void render(bool active);
    void destroy();

    void handleInput(int ch, bool& submitted, bool& cancelled);
    ConnectionConfig getConfig() const;
    void setConfig(const ConnectionConfig& config);

private:
    WINDOW* win_{nullptr};
    int height_{20};
    int width_{60};
    int starty_{0};
    int startx_{0};

    int activeField_{0};
    int modeIdx_{0}; // 0: Localhost, 1: Remote MySQL, 2: SQLite File, 3: Custom URI
    std::vector<std::string> modes_{"Localhost MySQL", "Online/Remote MySQL", "SQLite File", "Custom Connection URI"};

    std::string name_{"Local MySQL"};
    std::string host_{"127.0.0.1"};
    std::string port_{"3306"};
    std::string user_{"root"};
    std::string password_{""};
    std::string database_{""};
    std::string uriInput_{"mysql://root:password@127.0.0.1:3306/db"};
};

} // namespace dbterm

#endif // DBTERM_CONNECTIONDIALOG_H
