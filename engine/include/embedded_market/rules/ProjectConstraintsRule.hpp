#pragma once

#include "embedded_market/rules/Rule.hpp"

namespace embedded_market {

// Checks the user's project-wide constraints: every controller must meet the minimum clock,
// flash, and RAM, and every connection must use an allowed interface. Constraints the project
// does not declare are skipped.
class ProjectConstraintsRule : public ProjectRule {
public:
    std::string id() const override { return "project-constraints"; }
    std::string description() const override;
    std::vector<Finding> evaluate(const EvaluationContext& context) const override;
};

}  // namespace embedded_market
