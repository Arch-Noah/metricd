#include "metricd/core/Config.hpp"
#include <fstream>
#include <sstream>
#include <unistd.h>

namespace metricd {

    Config Config::defaults()
    {
        Config cfg;
        cfg.socket_path = std::string("/run/user/") + std::to_string(getuid()) + "/metricd.sock";
        cfg.interval = 1;
        cfg.enable_cpu = true;
        cfg.enable_memory = true;
        cfg.enable_disk = true;
        cfg.enable_network = true;
        cfg.enable_gpu = true;
        cfg.enable_per_core = false;
        return cfg;
    }

    Config Config::load(const std::string& path)
    {
        Config cfg = defaults();

        std::ifstream file(path);
        if (!file.is_open()) return cfg;

        std::string line;
        std::string current_section;

        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;

            if (line[0] == '[') {
                auto end = line.find(']');
                if (end != std::string::npos)
                    current_section = line.substr(1, end - 1);
                continue;
            }

            auto eq = line.find('=');
            if (eq == std::string::npos) continue;

            auto key_start = line.find_first_not_of(" \t");
            if (key_start == std::string::npos || key_start >= eq) continue;
            std::string key = line.substr(key_start, line.find_last_not_of(" \t", eq - 1) - key_start + 1);

            auto val_start = line.find_first_not_of(" \t", eq + 1);
            if (val_start == std::string::npos) continue;
            std::string val = line.substr(val_start);
            if (!val.empty() && val.back() == '\r') val.pop_back();

            bool quoted = (val.size() >= 2 && val.front() == '"' && val.back() == '"');
            std::string unquoted = quoted ? val.substr(1, val.size() - 2) : val;

            if (current_section.empty()) {
                if (key == "socket_path") cfg.socket_path = unquoted;
                else if (key == "interval") cfg.interval = std::stoi(val);
            } else if (current_section == "collectors") {
                bool enabled = (val == "true" || val == "yes" || val == "1");
                if (key == "cpu")       cfg.enable_cpu = enabled;
                if (key == "memory")    cfg.enable_memory = enabled;
                if (key == "disk")      cfg.enable_disk = enabled;
                if (key == "network")   cfg.enable_network = enabled;
                if (key == "gpu")       cfg.enable_gpu = enabled;
                if (key == "per_core")  cfg.enable_per_core = enabled;
            }
        }
        return cfg;
    }
}
