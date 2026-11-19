#include "embedded_market/engine/CompatibilityEngine.hpp"

#include <exception>
#include <utility>

#include "embedded_market/engine/EvaluationContext.hpp"

namespace embedded_market {

namespace {

// Runs one rule invocation, turning an exception into a Warning so one bad rule cannot hide
// the results of the others.
template <typename Invoke>
void runRule(const Rule& rule, std::vector<std::string> subjectIds, Invoke&& invoke, Report& report) {
    try {
        for (Finding& finding : invoke()) report.add(std::move(finding));
    } catch (const std::exception& e) {
        report.add(Finding{Severity::Warning, rule.id(), std::move(subjectIds),
                           "Rule '" + rule.id() + "' failed: " + e.what() + "; manual review required"});
    }
}

}  // namespace

CompatibilityEngine::CompatibilityEngine(RuleRegistry registry, EngineSettings settings)
    : registry_(std::move(registry)), settings_(settings) {}

Report CompatibilityEngine::evaluate(const Project& project) const {
    const EvaluationContext context(project, settings_);
    Report report;

    for (const Component& component : project.components()) {
        for (const auto& rule : registry_.componentRules()) {
            runRule(*rule, {component.id()}, [&] { return rule->evaluate(component, context); }, report);
        }
    }

    for (const Connection& connection : project.connections()) {
        // Project guarantees both endpoints exist.
        const Component& upstream = *project.findComponent(connection.upstream());
        const Component& downstream = *project.findComponent(connection.downstream());
        for (const auto& rule : registry_.connectionRules()) {
            runRule(*rule, {upstream.id(), downstream.id()},
                    [&] { return rule->evaluate(connection, upstream, downstream, context); }, report);
        }
    }

    for (const auto& rule : registry_.projectRules()) {
        runRule(*rule, {}, [&] { return rule->evaluate(context); }, report);
    }

    return report;
}

}  // namespace embedded_market
