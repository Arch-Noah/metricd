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
#include <fstream>
#include <string_view>
#include <cstdlib>
#include <thread>
#include <chrono>

struct CpuTime {
    unsigned long long idle = 0;
    unsigned long long total = 0;
};

static CpuTime getCpuSnapshot()
{
    std::ifstream statFile("/proc/stat");
    char buffer[256];

    if (!statFile.getline(buffer, sizeof(buffer))) {
        return {};
    }

    std::string_view line(buffer);
    if (line.rfind("cpu ", 0) != 0) return {};

    char* ptr = buffer + 4;
    char* next_ptr = nullptr;

    const unsigned long long user    = std::strtoull(ptr, &next_ptr, 10); ptr = next_ptr;
    const unsigned long long nice    = std::strtoull(ptr, &next_ptr, 10); ptr = next_ptr;
    const unsigned long long system  = std::strtoull(ptr, &next_ptr, 10); ptr = next_ptr;
    const unsigned long long idle    = std::strtoull(ptr, &next_ptr, 10); ptr = next_ptr;
    const unsigned long long iowait  = std::strtoull(ptr, &next_ptr, 10); ptr = next_ptr;
    const unsigned long long irq     = std::strtoull(ptr, &next_ptr, 10); ptr = next_ptr;
    const unsigned long long softirq = std::strtoull(ptr, &next_ptr, 10);

    const unsigned long long idle_total = idle + iowait;
    const unsigned long long non_idle   = user + nice + system + irq + softirq;
    const unsigned long long total      = idle_total + non_idle;

    return {idle_total, total};
}

nlohmann::json CpuCollector::collect()
{
    const CpuTime s1 = getCpuSnapshot();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    const CpuTime s2 = getCpuSnapshot();

    const double total_diff = static_cast<double>(s2.total - s1.total);
    const double idle_diff  = static_cast<double>(s2.idle - s1.idle);

    double usage_percent = 0.0;
    if (total_diff > 0.0) {
        usage_percent = ((total_diff - idle_diff) / total_diff) * 100.0;
    }

    return {
        {"cpu_usage_percent", usage_percent}
    };
}
