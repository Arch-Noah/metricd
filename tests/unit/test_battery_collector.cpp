#include "metricd/collectors/BatteryCollector.hpp"
#include <nlohmann/json.hpp>
#include <cassert>
#include <iostream>

int main()
{
    BatteryCollector collector;
    nlohmann::json j = collector.collect();

    assert(j.contains("batteries"));
    assert(j["batteries"].is_array());

    for (const auto& bat : j["batteries"]) {
        assert(bat.contains("name"));
        assert(bat["name"].is_string());
        assert(!bat["name"].get<std::string>().empty());

        if (bat.contains("capacity_percent")) {
            int cap = bat["capacity_percent"];
            assert(cap >= 0 && cap <= 100);
        }

        if (bat.contains("status")) {
            std::string s = bat["status"];
            assert(s == "Charging" || s == "Discharging" || s == "Full" ||
                   s == "Not charging" || s == "Unknown");
        }

        if (bat.contains("voltage_now"))
            assert(bat["voltage_now"].get<double>() > 0);

        if (bat.contains("temp_c"))
            assert(bat["temp_c"].get<double>() >= 0 && bat["temp_c"].get<double>() <= 100);

        if (bat.contains("cycle_count"))
            assert(bat["cycle_count"].get<long long>() >= 0);
    }

    std::cout << "battery collector: " << j.dump(2) << std::endl;
    std::cout << "OK" << std::endl;
    return 0;
}
