#pragma once

#include "metricd/collectors/ICollector.hpp"

class MemoryCollector final : public ICollector {
public:
    [[nodiscard]] nlohmann::json collect() override;
    [[nodiscard]] std::string name() const override { return "memory"; }
};
