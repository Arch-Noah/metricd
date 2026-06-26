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
#include <cassert>
#include <iostream>

int main()
{
    NetworkCollector collector;
    const auto json = collector.collect();

    assert(json.contains("interface"));
    assert(json.contains("download_speed_mb_s"));
    assert(json.contains("upload_speed_mb_s"));

    const double dl = json["download_speed_mb_s"].get<double>();
    const double ul = json["upload_speed_mb_s"].get<double>();
    assert(dl >= 0.0);
    assert(ul >= 0.0);

    std::cout << "PASS: NetworkCollector (interface=" << json["interface"] << ")" << std::endl;
    return 0;
}
