#include "metricd/collectors/TemperatureCollector.hpp"
#include <nlohmann/json.hpp>
#include <cassert>
#include <iostream>

int main()
{
    TemperatureCollector collector;
    const auto json = collector.collect();

    assert(json.contains("sensors"));
    assert(json["sensors"].is_array());

    if (!json["sensors"].empty()) {
        const auto& sensor = json["sensors"][0];
        assert(sensor.contains("label"));
        assert(sensor.contains("temp_c"));

        const double temp = sensor["temp_c"].get<double>();
        assert(temp >= 0.0);
        assert(temp < 200.0);
    }

    std::cout << "PASS: TemperatureCollector (" << json["sensors"].size() << " sensors)" << std::endl;
    return 0;
}
