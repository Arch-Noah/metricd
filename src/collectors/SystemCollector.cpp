#include "metricd/collectors/SystemCollector.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <string>
#include <cmath>

nlohmann::json SystemCollector::collect()
{
    nlohmann::json j;

    std::ifstream la("/proc/loadavg");
    if (la.is_open()) {
        double l1 = 0, l5 = 0, l15 = 0;
        unsigned int running = 0, total = 0;
        la >> l1 >> l5 >> l15;
        la.ignore(1, ' ');
        la >> running;
        la.ignore(1, '/');
        la >> total;

        j["load_1min"] = std::round(l1 * 100.0) / 100.0;
        j["load_5min"] = std::round(l5 * 100.0) / 100.0;
        j["load_15min"] = std::round(l15 * 100.0) / 100.0;
        j["procs_running"] = running;
        j["procs_total"] = total;
    }

    std::ifstream up("/proc/uptime");
    if (up.is_open()) {
        double uptime_secs = 0, idle_secs = 0;
        up >> uptime_secs >> idle_secs;

        j["uptime_seconds"] = static_cast<long long>(std::floor(uptime_secs));

        long long days = static_cast<long long>(uptime_secs) / 86400;
        long long hours = (static_cast<long long>(uptime_secs) % 86400) / 3600;
        long long mins = (static_cast<long long>(uptime_secs) % 3600) / 60;

        std::string human;
        if (days > 0) human += std::to_string(days) + "d ";
        human += std::to_string(hours) + "h" + std::to_string(mins) + "m";
        j["uptime_human"] = human;
    }

    return j;
}
