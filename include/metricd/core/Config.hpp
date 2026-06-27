#pragma once

#include <string>

namespace metricd {

struct Config {
    std::string socket_path;
    int interval = 1;
    bool enable_cpu     = true;
    bool enable_memory  = true;
    bool enable_disk    = true;
    bool enable_network = true;
    bool enable_gpu     = true;
    bool enable_per_core = false;

    static Config load(const std::string& path);
    static Config defaults();
};

}
