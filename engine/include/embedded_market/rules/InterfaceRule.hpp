#pragma once

#include "embedded_market/rules/Rule.hpp"

namespace embedded_market {

// Both ends of a connection must support the connection's chosen interface.
class InterfaceRule : public ConnectionRule {
public:
    std::string id() const override { return "interface"; }
    std::string description() const override;
    std::vector<Finding> evaluate(const Connection& connection,
                                  const Component& upstream,
                                  const Component& downstream,
                                  const EvaluationContext& context) const override;
};

}  // namespace embedded_market
