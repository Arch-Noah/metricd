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


#include "metricd/collectors/CpuCollector.hpp"
#include <nlohmann/json.hpp>
#include <cassert>
#include <iostream>

int main()
{
    CpuCollector collector;
    const auto json = collector.collect();

    assert(json.contains("cpu_usage_percent"));
    const double usage = json["cpu_usage_percent"].get<double>();
    assert(usage >= 0.0 && usage <= 100.0);

    std::cout << "PASS: CpuCollector (" << usage << "%)" << std::endl;
    return 0;
}
