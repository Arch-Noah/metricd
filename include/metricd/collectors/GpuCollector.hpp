#pragma once

#include "metricd/collectors/ICollector.hpp"
#include <nlohmann/json.hpp>
#include <chrono>

class GpuCollector final : public ICollector {
public:
    GpuCollector();
    [[nodiscard]] nlohmann::json collect() override;
    [[nodiscard]] std::string name() const override { return "gpu"; }

private:
    std::chrono::steady_clock::time_point last_collect_;
    nlohmann::json cached_;
    static constexpr auto CACHE_DURATION = std::chrono::seconds(5);

    static nlohmann::json queryNvidiaSmi();
};
