#include "metricd/collectors/TemperatureCollector.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <cstdio>
#include <cstring>
#include <unordered_map>

namespace fs = std::filesystem;

std::vector<TemperatureCollector::TempSensor> TemperatureCollector::readHwmon()
{
    std::vector<TempSensor> sensors;

    for (int hwmon = 0; hwmon < 16; ++hwmon) {
        char path[64];
        std::snprintf(path, sizeof(path), "/sys/class/hwmon/hwmon%d", hwmon);

        if (!fs::is_directory(path)) break;

        std::unordered_map<int, std::string> labels;
        for (int i = 1; i <= 32; ++i) {
            char labelPath[96];
            std::snprintf(labelPath, sizeof(labelPath), "%s/temp%d_label", path, i);

            std::ifstream lf(labelPath);
            if (!lf.is_open()) continue;
            std::string label;
            std::getline(lf, label);
            if (!label.empty()) {
                labels[i] = label;
            }
        }

        for (int i = 1; i <= 32; ++i) {
            char inputPath[96];
            std::snprintf(inputPath, sizeof(inputPath), "%s/temp%d_input", path, i);

            std::ifstream tf(inputPath);
            if (!tf.is_open()) break;

            unsigned long raw = 0;
            tf >> raw;
            if (raw == 0) continue;

            auto it = labels.find(i);
            std::string label = (it != labels.end()) ? it->second : "unknown";
            sensors.push_back({label, static_cast<double>(raw) / 1000.0});
        }
    }

    return sensors;
}

std::vector<TemperatureCollector::TempSensor> TemperatureCollector::readThermalZones()
{
    std::vector<TempSensor> sensors;

    for (int i = 0; i < 16; ++i) {
        char path[64];
        std::snprintf(path, sizeof(path), "/sys/class/thermal/thermal_zone%d/temp", i);

        std::ifstream tf(path);
        if (!tf.is_open()) break;

        unsigned long raw = 0;
        tf >> raw;
        if (raw == 0) continue;

        char typePath[64];
        std::snprintf(typePath, sizeof(typePath), "/sys/class/thermal/thermal_zone%d/type", i);
        std::ifstream typeFile(typePath);
        std::string label;
        if (typeFile.is_open()) {
            std::getline(typeFile, label);
        }
        if (label.empty()) label = "thermal_zone" + std::to_string(i);

        sensors.push_back({label, static_cast<double>(raw) / 1000.0});
    }

    return sensors;
}

nlohmann::json TemperatureCollector::collect()
{
    auto sensors = readHwmon();

    if (sensors.empty()) {
        sensors = readThermalZones();
    }

    nlohmann::json arr = nlohmann::json::array();
    for (const auto& s : sensors) {
        arr.push_back({{"label", s.label}, {"temp_c", s.temp_c}});
    }

    return {{"sensors", arr}};
}
