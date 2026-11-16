#pragma once

#include "embedded_market/rules/Rule.hpp"

namespace embedded_market {

// FR-02: the entire declared project supply range must fall within each component's operating
// range (inclusive, within EngineSettings::voltageTolerance). Overlap alone is a Conflict.
class SupplyVoltageRule : public ComponentRule {
public:
    std::string id() const override { return "supply-voltage"; }
    std::string description() const override;
    std::vector<Finding> evaluate(const Component& component, const EvaluationContext& context) const override;
};

}  // namespace embedded_market
