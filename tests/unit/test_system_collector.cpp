#include "metricd/collectors/SystemCollector.hpp"
#include <nlohmann/json.hpp>
#include <cassert>
#include <iostream>

int main()
{
    SystemCollector collector;
    nlohmann::json j = collector.collect();

    if (j.contains("load_1min")) {
        assert(j["load_1min"].is_number());
        assert(j["load_1min"].get<double>() >= 0);

        assert(j.contains("load_5min"));
        assert(j.contains("load_15min"));
        assert(j.contains("procs_running"));
        assert(j.contains("procs_total"));
        assert(j["procs_running"].get<unsigned int>() <= j["procs_total"].get<unsigned int>());
    }

    if (j.contains("uptime_seconds")) {
        assert(j["uptime_seconds"].get<long long>() > 0);
        assert(j.contains("uptime_human"));
    }

    std::cout << "system collector: " << j.dump(2) << std::endl;
    std::cout << "OK" << std::endl;
    return 0;
}
