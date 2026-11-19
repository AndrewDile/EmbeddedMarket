#pragma once

#include "embedded_market/engine/EngineSettings.hpp"
#include "embedded_market/engine/Report.hpp"
#include "embedded_market/model/Project.hpp"
#include "embedded_market/rules/RuleRegistry.hpp"

namespace embedded_market {

// Runs every registered rule over a project and collects the findings into a Report.
// A rule that throws is reported as a Warning rather than aborting the run.
class CompatibilityEngine {
public:
    explicit CompatibilityEngine(RuleRegistry registry = RuleRegistry::withDefaults(),
                                 EngineSettings settings = {});

    Report evaluate(const Project& project) const;

    const RuleRegistry& rules() const { return registry_; }
    const EngineSettings& settings() const { return settings_; }

private:
    RuleRegistry registry_;
    EngineSettings settings_;
};

}  // namespace embedded_market
