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


#include "metricd/collectors/NetworkCollector.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <string_view>
#include <cstdlib>
#include <thread>
#include <chrono>

struct NetSnapshot {
    unsigned long long rx_bytes = 0;
    unsigned long long tx_bytes = 0;
};

static NetSnapshot getNetworkSnapshot(const std::string& target_interface)
{
    std::ifstream netdev("/proc/net/dev");
    char buffer[320];

    if (!netdev.getline(buffer, sizeof(buffer)) || !netdev.getline(buffer, sizeof(buffer))) {
        return {};
    }

    while (netdev.getline(buffer, sizeof(buffer))) {
        std::string_view line(buffer);

        size_t pos = line.find(target_interface + ":");
        if (pos == std::string_view::npos) continue;

        char* ptr = buffer + pos + target_interface.length() + 1;
        char* next_ptr = nullptr;

        unsigned long long rx_bytes = std::strtoull(ptr, &next_ptr, 10);
        ptr = next_ptr;

        for (int i = 0; i < 7; ++i) {
            std::strtoull(ptr, &next_ptr, 10);
            ptr = next_ptr;
        }

        unsigned long long tx_bytes = std::strtoull(ptr, &next_ptr, 10);

        return {rx_bytes, tx_bytes};
    }
    return {};
}

nlohmann::json NetworkCollector::collect()
{
    const std::string interface = "eth0";

    NetSnapshot s1 = getNetworkSnapshot(interface);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    NetSnapshot s2 = getNetworkSnapshot(interface);

    unsigned long long d_rx_bytes = s2.rx_bytes - s1.rx_bytes;
    unsigned long long d_tx_bytes = s2.tx_bytes - s1.tx_bytes;

    double rx_mb_s = (static_cast<double>(d_rx_bytes) / (1024.0 * 1024.0)) / 0.1;
    double tx_mb_s = (static_cast<double>(d_tx_bytes) / (1024.0 * 1024.0)) / 0.1;

    return {
        {"interface", interface},
        {"download_speed_mb_s", rx_mb_s},
        {"upload_speed_mb_s", tx_mb_s}
    };
}
