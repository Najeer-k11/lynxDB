#ifndef DBTERM_CONNECTIONCONFIG_H
#define DBTERM_CONNECTIONCONFIG_H

#include <string>

namespace dbterm {

enum class DatabaseType {
    MYSQL,
    SQLITE
};

enum class ConnectionCategory {
    LOCAL_MYSQL,
    ONLINE_MYSQL,
    SQLITE_FILE,
    CUSTOM_URI
};

struct ConnectionConfig {
    std::string name{"Local MySQL"};
    DatabaseType type{DatabaseType::MYSQL};
    ConnectionCategory category{ConnectionCategory::LOCAL_MYSQL};
    std::string host{"127.0.0.1"};
    int port{3306};
    std::string user{"root"};
    std::string password{""};
    std::string database{""};
    std::string uriString{""};

    std::string getDisplayURI() const {
        if (type == DatabaseType::SQLITE) {
            return "sqlite://" + host;
        }
        std::string uri = "mysql://" + user;
        if (!password.empty()) {
            uri += ":****";
        }
        uri += "@" + host + ":" + std::to_string(port);
        if (!database.empty()) {
            uri += "/" + database;
        }
        return uri;
    }

    static ConnectionConfig parseURI(const std::string& uriStr, std::string& errOut) {
        ConnectionConfig cfg;
        cfg.uriString = uriStr;
        std::string s = uriStr;

        if (s.rfind("mysql://", 0) == 0) {
            cfg.type = DatabaseType::MYSQL;
            cfg.category = ConnectionCategory::ONLINE_MYSQL;
            s = s.substr(8);
        } else if (s.rfind("sqlite://", 0) == 0) {
            cfg.type = DatabaseType::SQLITE;
            cfg.category = ConnectionCategory::SQLITE_FILE;
            cfg.host = s.substr(9);
            return cfg;
        } else {
            errOut = "Invalid URI protocol. Expected mysql:// or sqlite://";
            return cfg;
        }

        auto atPos = s.find('@');
        if (atPos != std::string::npos) {
            std::string userPass = s.substr(0, atPos);
            s = s.substr(atPos + 1);
            auto colonPos = userPass.find(':');
            if (colonPos != std::string::npos) {
                cfg.user = userPass.substr(0, colonPos);
                cfg.password = userPass.substr(colonPos + 1);
            } else {
                cfg.user = userPass;
            }
        }

        auto slashPos = s.find('/');
        if (slashPos != std::string::npos) {
            cfg.database = s.substr(slashPos + 1);
            s = s.substr(0, slashPos);
        }

        auto colonPos = s.find(':');
        if (colonPos != std::string::npos) {
            cfg.host = s.substr(0, colonPos);
            try {
                cfg.port = std::stoi(s.substr(colonPos + 1));
            } catch (...) {
                cfg.port = 3306;
            }
        } else {
            cfg.host = s;
            cfg.port = 3306;
        }

        cfg.name = cfg.host + ":" + std::to_string(cfg.port);
        return cfg;
    }
};

} // namespace dbterm

#endif // DBTERM_CONNECTIONCONFIG_H
