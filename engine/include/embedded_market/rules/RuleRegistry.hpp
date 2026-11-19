#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "embedded_market/rules/Rule.hpp"

namespace embedded_market {

// Owns the rules the engine runs, grouped by scope. Rule ids must be unique.
class RuleRegistry {
public:
    // Registry with every built-in rule.
    static RuleRegistry withDefaults();

    // Each throws std::invalid_argument on a null rule or a duplicate id.
    void add(std::unique_ptr<ComponentRule> rule);
    void add(std::unique_ptr<ConnectionRule> rule);
    void add(std::unique_ptr<ProjectRule> rule);

    template <typename RuleT, typename... Args>
    RuleT& emplace(Args&&... args) {
        auto rule = std::make_unique<RuleT>(std::forward<Args>(args)...);
        RuleT& ref = *rule;
        add(std::move(rule));
        return ref;
    }

    bool contains(const std::string& ruleId) const;
    std::size_t size() const;

    const std::vector<std::unique_ptr<ComponentRule>>& componentRules() const { return componentRules_; }
    const std::vector<std::unique_ptr<ConnectionRule>>& connectionRules() const { return connectionRules_; }
    const std::vector<std::unique_ptr<ProjectRule>>& projectRules() const { return projectRules_; }

private:
    void checkNewRule(const Rule* rule) const;

    std::vector<std::unique_ptr<ComponentRule>> componentRules_;
    std::vector<std::unique_ptr<ConnectionRule>> connectionRules_;
    std::vector<std::unique_ptr<ProjectRule>> projectRules_;
};

}  // namespace embedded_market
