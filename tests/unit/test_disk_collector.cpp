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


#include "metricd/collectors/DiskCollector.hpp"
#include <nlohmann/json.hpp>
#include <cassert>
#include <iostream>

int main()
{
    DiskCollector collector;
    const auto json = collector.collect();

    assert(json.contains("filesystems"));
    assert(json["filesystems"].is_array());

    if (!json["filesystems"].empty()) {
        const auto& fs = json["filesystems"][0];
        assert(fs.contains("mount"));
        assert(fs.contains("total_gb"));
        assert(fs.contains("used_gb"));
        assert(fs.contains("available_gb"));
        assert(fs.contains("used_percent"));

        const auto total = fs["total_gb"].get<double>();
        assert(total > 0.0);
    }

    std::cout << "PASS: DiskCollector (" << json["filesystems"].size() << " filesystems)" << std::endl;
    return 0;
}
