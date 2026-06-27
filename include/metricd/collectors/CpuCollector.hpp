#pragma once

#include "metricd/collectors/ICollector.hpp"
#include <vector>

class CpuCollector final : public ICollector {
public:
    explicit CpuCollector(bool enablePerCore = false);
    [[nodiscard]] nlohmann::json collect() override;
    [[nodiscard]] std::string name() const override { return "cpu"; }

private:
    struct Snapshot {
        unsigned long long idle = 0;
        unsigned long long total = 0;
    };

    bool has_prev_{false};
    Snapshot prev_aggregate_;
    std::vector<Snapshot> prev_per_core_;
    bool enable_per_core_;

    static Snapshot readAggregate();
    static std::vector<Snapshot> readPerCore();
    static int readThreadCount();
    static double readCpuMHz();
    static double readCpuTemp();
};
