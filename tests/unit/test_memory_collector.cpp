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

    const auto total = json["total_mb"].get<unsigned long>();
    assert(total > 0);

    const auto pct = json["used_percent"].get<double>();
    assert(pct >= 0.0);

    std::cout << "PASS: MemoryCollector (total=" << total << "MB, used=" << json["used_mb"] << "MB)" << std::endl;
    return 0;
}
