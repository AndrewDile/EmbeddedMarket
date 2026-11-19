#pragma once

#include <string>
#include <vector>

#include "embedded_market/engine/EvaluationContext.hpp"
#include "embedded_market/engine/Finding.hpp"
#include "embedded_market/model/Component.hpp"
#include "embedded_market/model/Connection.hpp"

namespace embedded_market {

// What a rule is evaluated against. To add a new scope (e.g. a shared bus, or a chain of
// connections through a multiplexer), add a value here, a matching abstract base class
// below, a list in RuleRegistry, and a loop in CompatibilityEngine::evaluate.
enum class RuleScope {
    Component,   // Once per component.
    Connection,  // Once per connection, with both endpoints resolved.
    Project,     // Once per project, for checks that span many parts.
};

// Base for every compatibility rule. Concrete rules derive from one of the scoped bases.
class Rule {
public:
    Rule() = default;
    Rule(const Rule&) = delete;
    Rule& operator=(const Rule&) = delete;
    virtual ~Rule() = default;

    virtual std::string id() const = 0;  // Stable, unique, kebab-case.
    virtual std::string description() const = 0;
    virtual RuleScope scope() const = 0;
};

class ComponentRule : public Rule {
public:
    RuleScope scope() const final { return RuleScope::Component; }
    virtual std::vector<Finding> evaluate(const Component& component,
                                          const EvaluationContext& context) const = 0;
};

class ConnectionRule : public Rule {
public:
    RuleScope scope() const final { return RuleScope::Connection; }
    virtual std::vector<Finding> evaluate(const Connection& connection,
                                          const Component& upstream,
                                          const Component& downstream,
                                          const EvaluationContext& context) const = 0;
};

class ProjectRule : public Rule {
public:
    RuleScope scope() const final { return RuleScope::Project; }
    virtual std::vector<Finding> evaluate(const EvaluationContext& context) const = 0;
};

}  // namespace embedded_market
