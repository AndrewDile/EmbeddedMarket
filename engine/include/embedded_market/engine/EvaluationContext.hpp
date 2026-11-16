#pragma once

#include "embedded_market/engine/EngineSettings.hpp"
#include "embedded_market/model/Project.hpp"

namespace embedded_market {

// Read-only view handed to every rule: the project being checked and the engine settings.
class EvaluationContext {
public:
    EvaluationContext(const Project& project, const EngineSettings& settings)
        : project_(project), settings_(settings) {}

    const Project& project() const { return project_; }
    const EngineSettings& settings() const { return settings_; }

private:
    const Project& project_;
    const EngineSettings& settings_;
};

}  // namespace embedded_market
