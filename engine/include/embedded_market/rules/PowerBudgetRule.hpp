#pragma once

#include "embedded_market/rules/Rule.hpp"

namespace embedded_market {

// The summed maximum current of every component must fit within the project's power budget.
// A part with an unknown draw gets its own Warning, and the project total stays incomplete: the
// known subtotal can prove the budget is exceeded, but never that it is met.
// Skipped when the project declares no budget.
class PowerBudgetRule : public ProjectRule {
public:
    std::string id() const override { return "power-budget"; }
    std::string description() const override;
    std::vector<Finding> evaluate(const EvaluationContext& context) const override;
};

}  // namespace embedded_market
