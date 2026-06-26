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

#include "metricd/core/Daemon.hpp"
#include "Logger.hpp"
#include <cstdlib>

int main()
{
    std::string config_path;
    const char* env_path = std::getenv("METRICD_CONFIG");
    if (env_path) {
        config_path = env_path;
    } else {
        const char* home = std::getenv("HOME");
        config_path = home ? std::string(home) + "/.config/metricd/metricd.toml"
                           : "";
    }

    metricd::Config cfg;
    if (!config_path.empty()) {
        cfg = metricd::Config::load(config_path);
    } else {
        cfg = metricd::Config::defaults();
    }

    const char* env_sock = std::getenv("METRICD_SOCKET");
    if (env_sock) cfg.socket_path = env_sock;
    const char* env_interval = std::getenv("METRICD_INTERVAL");
    if (env_interval) cfg.interval = std::stoi(env_interval);

    try {
        metricd::Daemon daemon(cfg);
        return daemon.run();
    } catch (const std::exception& e) {
        LOG(Logger::LogLevel::ERROR, "%s", e.what());
        return 1;
    }
}
