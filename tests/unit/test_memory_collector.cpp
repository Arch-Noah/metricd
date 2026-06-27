#include "metricd/collectors/MemoryCollector.hpp"
#include <nlohmann/json.hpp>
#include <cassert>
#include <iostream>

int main()
{
    MemoryCollector collector;
    const auto json = collector.collect();

    assert(json.contains("total_mb"));
    assert(json.contains("used_mb"));
    assert(json.contains("available_mb"));
    assert(json.contains("used_percent"));
    assert(json.contains("swap_total_mb"));
    assert(json.contains("swap_used_mb"));
    assert(json.contains("swap_used_percent"));

    const auto total = json["total_mb"].get<unsigned long>();
    assert(total > 0);

    const auto pct = json["used_percent"].get<double>();
    assert(pct >= 0.0);

    std::cout << "PASS: MemoryCollector (total=" << total << "MB, swap_total=" << json["swap_total_mb"] << "MB)" << std::endl;
    return 0;
}
