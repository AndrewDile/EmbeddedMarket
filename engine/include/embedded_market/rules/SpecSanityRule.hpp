#pragma once

#include "embedded_market/rules/Rule.hpp"

namespace embedded_market {

// Flags invalid values (non-finite, negative, inverted ranges) and unlikely values (outside
// SanityLimits) as Warnings. Missing values are left to the rules that need them.
class SpecSanityRule : public ComponentRule {
public:
    std::string id() const override { return "spec-sanity"; }
    std::string description() const override;
    std::vector<Finding> evaluate(const Component& component, const EvaluationContext& context) const override;
};

}  // namespace embedded_market
