#include "metricd/collectors/CpuCollector.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <string_view>
#include <cstdlib>
#include <cstdio>

CpuCollector::CpuCollector(bool enablePerCore)
    : enable_per_core_(enablePerCore)
{
}

CpuCollector::Snapshot CpuCollector::readAggregate()
{
    std::ifstream statFile("/proc/stat");
    char buffer[256];
    if (!statFile.getline(buffer, sizeof(buffer))) return {};

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
    return {idle_total, idle_total + non_idle};
}

std::vector<CpuCollector::Snapshot> CpuCollector::readPerCore()
{
    std::ifstream statFile("/proc/stat");
    char buffer[256];
    std::vector<Snapshot> cores;

    while (statFile.getline(buffer, sizeof(buffer))) {
        std::string_view line(buffer);
        if (line.rfind("cpu", 0) != 0 || line.size() < 4) continue;
        if (line[3] < '0' || line[3] > '9') continue;

        char* ptr = buffer;
        while (*ptr && *ptr != ' ') ++ptr;
        if (!*ptr) continue;
        ++ptr;

        char* next_ptr = nullptr;
        const unsigned long long user    = std::strtoull(ptr, &next_ptr, 10); ptr = next_ptr;
        const unsigned long long nice    = std::strtoull(ptr, &next_ptr, 10); ptr = next_ptr;
        const unsigned long long system  = std::strtoull(ptr, &next_ptr, 10); ptr = next_ptr;
        const unsigned long long idle    = std::strtoull(ptr, &next_ptr, 10); ptr = next_ptr;
        const unsigned long long iowait  = std::strtoull(ptr, &next_ptr, 10); ptr = next_ptr;
        const unsigned long long irq     = std::strtoull(ptr, &next_ptr, 10); ptr = next_ptr;
        const unsigned long long softirq = std::strtoull(ptr, &next_ptr, 10);

        const unsigned long long idle_total = idle + iowait;
        const unsigned long long total      = idle_total + user + nice + system + irq + softirq;
        cores.push_back({idle_total, total});
    }
    return cores;
}

int CpuCollector::readThreadCount()
{
    std::ifstream statFile("/proc/stat");
    char buffer[256];
    int count = 0;

    while (statFile.getline(buffer, sizeof(buffer))) {
        std::string_view line(buffer);
        if (line.rfind("cpu", 0) != 0 || line.size() < 4) continue;
        if (line[3] >= '0' && line[3] <= '9') ++count;
    }
    return count > 0 ? count : 1;
}

double CpuCollector::readCpuMHz()
{
    std::ifstream cpuinfo("/proc/cpuinfo");
    char buffer[256];
    while (cpuinfo.getline(buffer, sizeof(buffer))) {
        std::string_view line(buffer);
        if (line.rfind("cpu MHz", 0) == 0) {
            auto colon = line.find(':');
            if (colon != std::string_view::npos) {
                return std::strtod(buffer + colon + 1, nullptr);
            }
        }
    }
    return 0.0;
}

double CpuCollector::readCpuTemp()
{
    for (int i = 0; i < 8; ++i) {
        char path[64];
        std::snprintf(path, sizeof(path), "/sys/class/thermal/thermal_zone%d/temp", i);

        std::ifstream tempFile(path);
        if (!tempFile.is_open()) continue;

        unsigned long raw = 0;
        tempFile >> raw;
        if (raw > 0) return static_cast<double>(raw) / 1000.0;
    }
    return 0.0;
}

nlohmann::json CpuCollector::collect()
{
    const Snapshot agg = readAggregate();
    const double cpu_mhz = readCpuMHz();
    const double cpu_temp = readCpuTemp();
    const int threads = readThreadCount();

    double usage_percent = 0.0;
    if (has_prev_) {
        const double total_diff = static_cast<double>(agg.total - prev_aggregate_.total);
        const double idle_diff  = static_cast<double>(agg.idle - prev_aggregate_.idle);
        if (total_diff > 0.0) {
            usage_percent = ((total_diff - idle_diff) / total_diff) * 100.0;
        }
    }
    prev_aggregate_ = agg;

    nlohmann::json result;
    result["cpu_usage_percent"] = usage_percent;
    result["clock_ghz"] = cpu_mhz > 0 ? cpu_mhz / 1000.0 : 0.0;
    result["temp_c"] = cpu_temp;
    result["threads"] = threads;

    if (enable_per_core_) {
        nlohmann::json perCore = nlohmann::json::array();
        const auto cores = readPerCore();

        if (has_prev_ && prev_per_core_.size() == cores.size()) {
            for (size_t i = 0; i < cores.size(); ++i) {
                const double td = static_cast<double>(cores[i].total - prev_per_core_[i].total);
                const double id = static_cast<double>(cores[i].idle - prev_per_core_[i].idle);
                if (td > 0.0) {
                    perCore.push_back(((td - id) / td) * 100.0);
                } else {
                    perCore.push_back(0.0);
                }
            }
        } else {
            for (size_t i = 0; i < cores.size(); ++i) {
                perCore.push_back(0.0);
            }
        }
        prev_per_core_ = cores;
        result["per_core"] = perCore;
    }

    has_prev_ = true;
    return result;
}
