#include "metricd/collectors/MemoryCollector.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <string_view>
#include <cstdlib>

nlohmann::json MemoryCollector::collect()
{
    std::ifstream meminfo("/proc/meminfo");
    if (!meminfo.is_open()) {
        return nlohmann::json::object();
    }

    char buffer[128];

    unsigned long total_kb = 0;
    unsigned long available_kb = 0;
    unsigned long swap_total_kb = 0;
    unsigned long swap_free_kb = 0;
    int fields_found = 0;

    while (meminfo.getline(buffer, sizeof(buffer)) && fields_found < 4) {
        std::string_view line(buffer);

        if (line.rfind("MemTotal:", 0) == 0) {
            total_kb = std::strtoul(buffer + 9, nullptr, 10);
            fields_found++;
        } else if (line.rfind("MemAvailable:", 0) == 0) {
            available_kb = std::strtoul(buffer + 13, nullptr, 10);
            fields_found++;
        } else if (line.rfind("SwapTotal:", 0) == 0) {
            swap_total_kb = std::strtoul(buffer + 10, nullptr, 10);
            fields_found++;
        } else if (line.rfind("SwapFree:", 0) == 0) {
            swap_free_kb = std::strtoul(buffer + 9, nullptr, 10);
            fields_found++;
        }
    }

    const unsigned long used_kb = (total_kb > available_kb) ? (total_kb - available_kb) : 0;
    const unsigned long swap_used_kb = (swap_total_kb > swap_free_kb) ? (swap_total_kb - swap_free_kb) : 0;

    return {
        {"total_mb",     total_kb / 1024},
        {"used_mb",      used_kb / 1024},
        {"available_mb", available_kb / 1024},
        {"used_percent", total_kb > 0
            ? (static_cast<double>(used_kb) / static_cast<double>(total_kb)) * 100.0
            : 0.0},
        {"swap_total_mb", swap_total_kb / 1024},
        {"swap_used_mb", swap_used_kb / 1024},
        {"swap_used_percent", swap_total_kb > 0
            ? (static_cast<double>(swap_used_kb) / static_cast<double>(swap_total_kb)) * 100.0
            : 0.0}
    };
}
