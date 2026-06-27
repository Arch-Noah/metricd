#pragma once

#include "metricd/collectors/ICollector.hpp"
#include <string>
#include <unordered_map>

class NetworkCollector final : public ICollector {
public:
    NetworkCollector();
    [[nodiscard]] nlohmann::json collect() override;
    [[nodiscard]] std::string name() const override { return "network"; }

private:
    struct Snapshot {
        unsigned long long rx_bytes = 0;
        unsigned long long tx_bytes = 0;
    };

    bool has_prev_{false};
    std::unordered_map<std::string, Snapshot> prev_;
    std::unordered_map<std::string, Snapshot> start_;

    static std::unordered_map<std::string, Snapshot> readAllInterfaces();
};
