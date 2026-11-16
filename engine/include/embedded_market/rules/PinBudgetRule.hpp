#pragma once

#include "embedded_market/rules/Rule.hpp"

namespace embedded_market {

// FR-06: for every component with downstream connections, the pins those connections need must
// fit within its available GPIO. Pins are counted by interface (see interfacePins): bus signals
// once per host, per-device signals for each attached part, plus each part's extra pins.
// Each host is budgeted separately, so nested hosts (MCU -> motor driver -> motor) are each
// checked against their own pins. Pin types and specific pin reservations are not modeled yet.
class PinBudgetRule : public ProjectRule {
public:
    std::string id() const override { return "pin-budget"; }
    std::string description() const override;
    std::vector<Finding> evaluate(const EvaluationContext& context) const override;
};

}  // namespace embedded_market
