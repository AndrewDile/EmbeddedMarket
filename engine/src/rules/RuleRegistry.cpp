#include "embedded_market/rules/RuleRegistry.hpp"

#include <stdexcept>

#include "embedded_market/rules/InterfaceRule.hpp"
#include "embedded_market/rules/LogicLevelRule.hpp"
#include "embedded_market/rules/PinBudgetRule.hpp"
#include "embedded_market/rules/SpecSanityRule.hpp"
#include "embedded_market/rules/SupplyVoltageRule.hpp"

namespace embedded_market {

RuleRegistry RuleRegistry::withDefaults() {
    RuleRegistry registry;
    registry.emplace<SpecSanityRule>();
    registry.emplace<SupplyVoltageRule>();
    registry.emplace<InterfaceRule>();
    registry.emplace<LogicLevelRule>();
    registry.emplace<PinBudgetRule>();
    return registry;
}

void RuleRegistry::checkNewRule(const Rule* rule) const {
    if (!rule) throw std::invalid_argument("Cannot register a null rule");
    if (contains(rule->id())) throw std::invalid_argument("Duplicate rule id: " + rule->id());
}

void RuleRegistry::add(std::unique_ptr<ComponentRule> rule) {
    checkNewRule(rule.get());
    componentRules_.push_back(std::move(rule));
}

void RuleRegistry::add(std::unique_ptr<ConnectionRule> rule) {
    checkNewRule(rule.get());
    connectionRules_.push_back(std::move(rule));
}

void RuleRegistry::add(std::unique_ptr<ProjectRule> rule) {
    checkNewRule(rule.get());
    projectRules_.push_back(std::move(rule));
}

bool RuleRegistry::contains(const std::string& ruleId) const {
    auto matches = [&](const auto& rules) {
        for (const auto& rule : rules) {
            if (rule->id() == ruleId) return true;
        }
        return false;
    };
    return matches(componentRules_) || matches(connectionRules_) || matches(projectRules_);
}

std::size_t RuleRegistry::size() const {
    return componentRules_.size() + connectionRules_.size() + projectRules_.size();
}

}  // namespace embedded_market
