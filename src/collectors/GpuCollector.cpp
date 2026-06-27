#include "metricd/collectors/GpuCollector.hpp"
#include <nlohmann/json.hpp>
#include <cstdio>
#include <cstring>
#include <string>

GpuCollector::GpuCollector()
    : last_collect_(std::chrono::steady_clock::time_point::min())
{
}

nlohmann::json GpuCollector::queryNvidiaSmi()
{
    FILE* fp = popen(
        "nvidia-smi --query-gpu=utilization.gpu,memory.total,memory.used,"
        "temperature.gpu,name --format=csv,noheader,nounits 2>/dev/null",
        "r"
    );
    if (!fp) return nlohmann::json::object();

    char buffer[256];
    if (!fgets(buffer, sizeof(buffer), fp)) {
        pclose(fp);
        return nlohmann::json::object();
    }
    pclose(fp);

    size_t len = std::strlen(buffer);
    while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
        buffer[--len] = '\0';
    }

    double vals[4] = {0};
    int field = 0;
    char* token = buffer;
    char* end;

    for (int i = 0; i < 4; ++i) {
        while (*token == ' ') ++token;
        end = std::strchr(token, ',');
        if (!end) break;
        *end = '\0';
        vals[i] = std::strtod(token, nullptr);
        token = end + 1;
        field++;
    }

    while (*token == ' ') ++token;
    std::string label(token);

    if (field < 4) return nlohmann::json::object();

    return {
        {"load_percent", vals[0]},
        {"memory_total_gb", vals[1] / 1024.0},
        {"memory_used_gb", vals[2] / 1024.0},
        {"temp_c", vals[3]},
        {"label", label}
    };
}

nlohmann::json GpuCollector::collect()
{
    const auto now = std::chrono::steady_clock::now();
    if (now - last_collect_ < CACHE_DURATION && !cached_.is_null()) {
        return cached_;
    }

    last_collect_ = now;
    cached_ = queryNvidiaSmi();
    return cached_;
}
