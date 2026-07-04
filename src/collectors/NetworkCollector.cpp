#include "metricd/collectors/NetworkCollector.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <string_view>
#include <cstdlib>
#include <cstring>

NetworkCollector::NetworkCollector()
{
    start_ = readAllInterfaces();
}

std::unordered_map<std::string, NetworkCollector::Snapshot> NetworkCollector::readAllInterfaces()
{
    std::ifstream netdev("/proc/net/dev");
    char buffer[320];
    std::unordered_map<std::string, Snapshot> result;

    if (!netdev.getline(buffer, sizeof(buffer)) || !netdev.getline(buffer, sizeof(buffer))) {
        return result;
    }

    while (netdev.getline(buffer, sizeof(buffer))) {
        char* ptr = buffer;
        while (*ptr == ' ') ++ptr;
        char* colon = std::strchr(ptr, ':');
        if (!colon) continue;

        std::string ifname(ptr, colon - ptr);
        if (ifname == "lo") continue;

        char* num_ptr = colon + 1;
        char* next_ptr = nullptr;

        unsigned long long rx_bytes = std::strtoull(num_ptr, &next_ptr, 10);
        num_ptr = next_ptr;

        for (int i = 0; i < 7; ++i) {
            std::strtoull(num_ptr, &next_ptr, 10);
            num_ptr = next_ptr;
        }

        unsigned long long tx_bytes = std::strtoull(num_ptr, &next_ptr, 10);

        result.emplace(ifname, Snapshot{rx_bytes, tx_bytes});
    }
    return result;
}

nlohmann::json NetworkCollector::collect()
{
    const auto current = readAllInterfaces();
    nlohmann::json interfaces = nlohmann::json::array();

    for (const auto& [name, cur] : current) {
        nlohmann::json iface;
        iface["name"] = name;
        iface["rx_bytes_cumulative"] = cur.rx_bytes;
        iface["tx_bytes_cumulative"] = cur.tx_bytes;

        if (has_prev_) {
            auto prev_it = prev_.find(name);
            if (prev_it != prev_.end()) {
                const double d_rx = static_cast<double>(cur.rx_bytes - prev_it->second.rx_bytes);
                const double d_tx = static_cast<double>(cur.tx_bytes - prev_it->second.tx_bytes);
                iface["download_speed_bps"] = (d_rx >= 0.0 && d_rx < 1e15) ? d_rx : 0.0;
                iface["upload_speed_bps"]   = (d_tx >= 0.0 && d_tx < 1e15) ? d_tx : 0.0;
            } else {
                iface["download_speed_bps"] = 0.0;
                iface["upload_speed_bps"] = 0.0;
            }
        } else {
            iface["download_speed_bps"] = 0.0;
            iface["upload_speed_bps"] = 0.0;
        }

        auto start_it = start_.find(name);
        if (start_it != start_.end()) {
            iface["download_today_bytes"] = cur.rx_bytes > start_it->second.rx_bytes
                ? cur.rx_bytes - start_it->second.rx_bytes : 0ULL;
            iface["upload_today_bytes"] = cur.tx_bytes > start_it->second.tx_bytes
                ? cur.tx_bytes - start_it->second.tx_bytes : 0ULL;
        } else {
            iface["download_today_bytes"] = 0;
            iface["upload_today_bytes"] = 0;
        }

        interfaces.push_back(iface);
    }

    prev_ = current;
    has_prev_ = true;

    return {{"interfaces", interfaces}};
}
