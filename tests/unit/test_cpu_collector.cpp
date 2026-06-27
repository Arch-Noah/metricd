#include "metricd/collectors/CpuCollector.hpp"
#include <nlohmann/json.hpp>
#include <cassert>
#include <iostream>

int main()
{
    CpuCollector collector(true);
    const auto json = collector.collect();

    assert(json.contains("cpu_usage_percent"));
    assert(json.contains("clock_ghz"));
    assert(json.contains("threads"));
    assert(json.contains("temp_c"));
    assert(json.contains("per_core"));

    const double usage = json["cpu_usage_percent"].get<double>();
    assert(usage >= 0.0 && usage <= 100.0);

    const int threads = json["threads"].get<int>();
    assert(threads > 0);

    const auto& per_core = json["per_core"];
    assert(per_core.is_array());
    assert(per_core.size() > 0);

    std::cout << "PASS: CpuCollector (" << usage << "%, " << threads << " threads)" << std::endl;
    return 0;
}
