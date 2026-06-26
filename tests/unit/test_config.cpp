/*
**  _                                              _      ___    ___  
** | |                                            | |    |__ \  / _ \
** | |_Created _       _ __   _ __    ___    __ _ | |__     ) || (_) |
** | '_ \ | | | |     | '_ \ | '_ \  / _ \  / _` || '_ \   / /  \__, |
** | |_) || |_| |     | | | || | | || (_) || (_| || | | | / /_    / /
** |_.__/  \__, |     |_| |_||_| |_| \___/  \__,_||_| |_||____|  /_/
**          __/ |     on 25/06/2026.
**         |___/
*/


#include "metricd/core/Config.hpp"
#include <cassert>
#include <iostream>

int main()
{
    {
        const auto cfg = metricd::Config::defaults();
        assert(!cfg.socket_path.empty());
        assert(cfg.interval == 1);
        assert(cfg.enable_cpu);
        assert(cfg.enable_memory);
        assert(cfg.enable_disk);
        assert(cfg.enable_network);
        std::cout << "PASS: Config defaults" << std::endl;
    }

    {
        const auto cfg = metricd::Config::load("config/metricd.default.toml");
        assert(!cfg.socket_path.empty());
        assert(cfg.interval == 1);
        assert(cfg.enable_cpu);
        assert(cfg.enable_memory);
        assert(cfg.enable_disk);
        assert(cfg.enable_network);
        std::cout << "PASS: Config load from file" << std::endl;
    }

    {
        const auto cfg = metricd::Config::load("/nonexistent/path.toml");
        assert(!cfg.socket_path.empty());
        assert(cfg.interval == 1);
        std::cout << "PASS: Config fallback to defaults on missing file" << std::endl;
    }

    return 0;
}
