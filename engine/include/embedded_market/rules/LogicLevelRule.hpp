#pragma once

#include "embedded_market/rules/Rule.hpp"

namespace embedded_market {

// Checks logic levels in both directions of a connection, since I2C, SPI, and UART all have
// signals driven by each side. For each driver -> receiver direction:
//   - driver V_OH above the receiver's maximum input voltage is a Conflict (risk of damage);
//   - driver V_OH below the receiver's V_IH(min) is a Conflict (highs may not register).
// Open-drain pull-up levels (e.g. I2C) are not modeled yet.
class LogicLevelRule : public ConnectionRule {
public:
    std::string id() const override { return "logic-level"; }
    std::string description() const override;
    std::vector<Finding> evaluate(const Connection& connection,
                                  const Component& upstream,
                                  const Component& downstream,
                                  const EvaluationContext& context) const override;
};

}  // namespace embedded_market
