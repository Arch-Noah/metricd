#include "metricd/collectors/BatteryCollector.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <cstdlib>
#include <climits>

namespace fs = std::filesystem;

std::vector<fs::path> BatteryCollector::findBatteries()
{
    std::vector<fs::path> batteries;
    fs::path dir("/sys/class/power_supply");

    if (!fs::is_directory(dir))
        return batteries;

    for (const auto& entry : fs::directory_iterator(dir)) {
        fs::path type_file = entry.path() / "type";
        std::ifstream tf(type_file);
        if (!tf.is_open()) continue;
        std::string type;
        std::getline(tf, type);
        if (type == "Battery")
            batteries.push_back(entry.path());
    }

    return batteries;
}

std::string BatteryCollector::readString(const fs::path& path)
{
    std::ifstream f(path);
    if (!f.is_open()) return {};
    std::string val;
    std::getline(f, val);
    return val;
}

long long BatteryCollector::readLL(const fs::path& path)
{
    std::ifstream f(path);
    if (!f.is_open()) return LLONG_MIN;
    long long val = 0;
    f >> val;
    if (f.fail()) return LLONG_MIN;
    return val;
}

nlohmann::json BatteryCollector::readBattery(const fs::path& dir)
{
    nlohmann::json bat;

    std::string name = dir.filename().string();
    bat["name"] = name;

    long long capacity = readLL(dir / "capacity");
    if (capacity != LLONG_MIN)
        bat["capacity_percent"] = capacity;

    std::string status = readString(dir / "status");
    if (!status.empty())
        bat["status"] = status;

    const long long voltage_now = readLL(dir / "voltage_now");
    if (voltage_now != LLONG_MIN)
        bat["voltage_now"] = static_cast<double>(voltage_now) / 1e6;

    const long long current_now = readLL(dir / "current_now");
    if (current_now != LLONG_MIN)
        bat["current_now"] = static_cast<double>(current_now) / 1e6;

    const long long power_now = readLL(dir / "power_now");
    if (power_now != LLONG_MIN)
        bat["power_now"] = static_cast<double>(power_now) / 1e6;

    const long long energy_now = readLL(dir / "energy_now");
    if (energy_now != LLONG_MIN)
        bat["energy_now"] = static_cast<double>(energy_now) / 1e6;

    const long long energy_full = readLL(dir / "energy_full");
    if (energy_full != LLONG_MIN)
        bat["energy_full"] = static_cast<double>(energy_full) / 1e6;

    const long long energy_full_design = readLL(dir / "energy_full_design");
    if (energy_full_design != LLONG_MIN)
        bat["energy_full_design"] = static_cast<double>(energy_full_design) / 1e6;

    const long long charge_full = readLL(dir / "charge_full");
    if (charge_full != LLONG_MIN)
        bat["charge_full"] = static_cast<double>(charge_full) / 1e6;

    long long charge_full_design = readLL(dir / "charge_full_design");
    if (charge_full_design != LLONG_MIN)
        bat["charge_full_design"] = static_cast<double>(charge_full_design) / 1e6;

    const long long charge_now = readLL(dir / "charge_now");
    if (charge_now != LLONG_MIN)
        bat["charge_now"] = static_cast<double>(charge_now) / 1e6;

    const long long temp_raw = readLL(dir / "temp");
    if (temp_raw != LLONG_MIN)
        bat["temp_c"] = static_cast<double>(temp_raw) / 10.0;

    long long cycle_count = readLL(dir / "cycle_count");
    if (cycle_count != LLONG_MIN)
        bat["cycle_count"] = cycle_count;

    std::string model = readString(dir / "model_name");
    if (!model.empty())
        bat["model"] = model;

    std::string manufacturer = readString(dir / "manufacturer");
    if (!manufacturer.empty())
        bat["manufacturer"] = manufacturer;

    std::string technology = readString(dir / "technology");
    if (!technology.empty())
        bat["technology"] = technology;

    if (energy_full_design != LLONG_MIN && energy_full != LLONG_MIN && energy_full_design > 0) {
        const double health = (static_cast<double>(energy_full) / energy_full_design) * 100.0;
        bat["health_percent"] = std::round(health * 10.0) / 10.0;
    } else if (charge_full_design != LLONG_MIN && charge_full != LLONG_MIN && charge_full_design > 0) {
        const double health = (static_cast<double>(charge_full) / charge_full_design) * 100.0;
        bat["health_percent"] = std::round(health * 10.0) / 10.0;
    }

    return bat;
}

nlohmann::json BatteryCollector::collect()
{
    const auto batteries = findBatteries();

    nlohmann::json arr = nlohmann::json::array();
    for (const auto& b : batteries) {
        arr.push_back(readBattery(b));
    }

    return {{"batteries", arr}};
}
