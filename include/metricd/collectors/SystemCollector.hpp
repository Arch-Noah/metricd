#pragma once

#include "metricd/collectors/ICollector.hpp"
#include <nlohmann/json.hpp>

class SystemCollector final : public ICollector {
public:
    [[nodiscard]] nlohmann::json collect() override;
    [[nodiscard]] std::string name() const override { return "system"; }
};
