#ifndef DBTERM_CONFIGMANAGER_H
#define DBTERM_CONFIGMANAGER_H

#include "models/ConnectionConfig.h"
#include <vector>
#include <string>
#include <fstream>
#include <cstdlib>
#include <sys/stat.h>

namespace dbterm {

class ConfigManager {
public:
    static std::string getConfigDirPath() {
        const char* home = std::getenv("HOME");
        std::string base = home ? home : ".";
        return base + "/.config/lynxdb";
    }

    static std::string getConfigFilePath() {
        return getConfigDirPath() + "/connections.cfg";
    }

    static void saveConnections(const std::vector<ConnectionConfig>& configs) {
        std::string dir = getConfigDirPath();
        mkdir(dir.c_str(), 0755);

        std::ofstream out(getConfigFilePath());
        if (!out.is_open()) return;

        for (const auto& cfg : configs) {
            out << "[Connection]\n";
            out << "Name=" << cfg.name << "\n";
            out << "Type=" << (cfg.type == DatabaseType::MYSQL ? "MYSQL" : "SQLITE") << "\n";
            out << "Host=" << cfg.host << "\n";
            out << "Port=" << cfg.port << "\n";
            out << "User=" << cfg.user << "\n";
            out << "Password=" << cfg.password << "\n";
            out << "Database=" << cfg.database << "\n";
            out << "\n";
        }
    }

    static std::vector<ConnectionConfig> loadConnections() {
        std::vector<ConnectionConfig> configs;
        std::ifstream in(getConfigFilePath());
        if (!in.is_open()) return configs;

        std::string line;
        ConnectionConfig cur;
        bool inConfig = false;

        while (std::getline(in, line)) {
            if (line == "[Connection]") {
                if (inConfig) configs.push_back(cur);
                cur = ConnectionConfig();
                inConfig = true;
            } else if (inConfig) {
                auto eq = line.find('=');
                if (eq != std::string::npos) {
                    std::string key = line.substr(0, eq);
                    std::string val = line.substr(eq + 1);
                    if (key == "Name") cur.name = val;
                    else if (key == "Type") cur.type = (val == "SQLITE" ? DatabaseType::SQLITE : DatabaseType::MYSQL);
                    else if (key == "Host") cur.host = val;
                    else if (key == "Port") { try { cur.port = std::stoi(val); } catch (...) {} }
                    else if (key == "User") cur.user = val;
                    else if (key == "Password") cur.password = val;
                    else if (key == "Database") cur.database = val;
                }
            }
        }
        if (inConfig) configs.push_back(cur);

        return configs;
    }

    static void saveOrUpdateConnection(const ConnectionConfig& newCfg) {
        auto configs = loadConnections();
        bool exists = false;
        for (auto& cfg : configs) {
            if (cfg.host == newCfg.host && cfg.port == newCfg.port && cfg.database == newCfg.database && cfg.user == newCfg.user) {
                cfg = newCfg;
                exists = true;
                break;
            }
        }
        if (!exists) {
            configs.push_back(newCfg);
        }
        saveConnections(configs);
    }

    static bool removeConnection(size_t index) {
        auto configs = loadConnections();
        if (index < configs.size()) {
            configs.erase(configs.begin() + index);
            saveConnections(configs);
            return true;
        }
        return false;
    }
};

} // namespace dbterm

#endif // DBTERM_CONFIGMANAGER_H
