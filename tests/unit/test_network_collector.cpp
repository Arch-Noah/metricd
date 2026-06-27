#include "metricd/collectors/NetworkCollector.hpp"
#include <nlohmann/json.hpp>
#include <cassert>
#include <iostream>

int main()
{
    NetworkCollector collector;
    const auto json = collector.collect();

    assert(json.contains("interfaces"));
    assert(json["interfaces"].is_array());

    if (!json["interfaces"].empty()) {
        const auto& iface = json["interfaces"][0];
        assert(iface.contains("name"));
        assert(iface.contains("download_speed_bps"));
        assert(iface.contains("upload_speed_bps"));
        assert(iface.contains("rx_bytes_cumulative"));
        assert(iface.contains("tx_bytes_cumulative"));
        assert(iface.contains("download_today_bytes"));
        assert(iface.contains("upload_today_bytes"));

        const double dl = iface["download_speed_bps"].get<double>();
        const double ul = iface["upload_speed_bps"].get<double>();
        assert(dl >= 0.0);
        assert(ul >= 0.0);
    }

    std::cout << "PASS: NetworkCollector (" << json["interfaces"].size() << " interfaces)" << std::endl;
    return 0;
}
