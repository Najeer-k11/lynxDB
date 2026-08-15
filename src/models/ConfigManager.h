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
    static std::string encryptPassword(const std::string& rawPass) {
        if (rawPass.empty()) return "";
        std::string key = "lynxDB_Secret_Key_2026";
        std::string obfuscated = rawPass;
        for (size_t i = 0; i < rawPass.size(); ++i) {
            obfuscated[i] = rawPass[i] ^ key[i % key.size()];
        }
        static const char b64table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string out;
        int val = 0, valb = -6;
        for (unsigned char c : obfuscated) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                out.push_back(b64table[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6) out.push_back(b64table[((val << 8) >> (valb + 8)) & 0x3F]);
        while (out.size() % 4) out.push_back('=');
        return "enc:" + out;
    }

    static std::string decryptPassword(const std::string& encPass) {
        if (encPass.empty()) return "";
        if (encPass.rfind("enc:", 0) != 0) return encPass; // Fallback for plain-text legacy passwords

        std::string b64 = encPass.substr(4);
        std::vector<int> T(256, -1);
        static const char b64table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        for (int i = 0; i < 64; i++) T[static_cast<unsigned char>(b64table[i])] = i;

        std::string decoded;
        int val = 0, valb = -8;
        for (unsigned char c : b64) {
            if (T[c] == -1) break;
            val = (val << 6) + T[c];
            valb += 6;
            if (valb >= 0) {
                decoded.push_back(char((val >> valb) & 0xFF));
                valb -= 8;
            }
        }

        std::string key = "lynxDB_Secret_Key_2026";
        std::string raw = decoded;
        for (size_t i = 0; i < decoded.size(); ++i) {
            raw[i] = decoded[i] ^ key[i % key.size()];
        }
        return raw;
    }

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
            out << "Password=" << encryptPassword(cfg.password) << "\n";
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
                    else if (key == "Password") cur.password = decryptPassword(val);
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
