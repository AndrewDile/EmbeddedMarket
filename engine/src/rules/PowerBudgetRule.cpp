#include "embedded_market/rules/PowerBudgetRule.hpp"

#include <sstream>

#include "embedded_market/rules/SpecGate.hpp"
#include "embedded_market/util/Format.hpp"

namespace embedded_market {

namespace {

std::string formatMilliamps(double milliamps) {
    std::ostringstream out;
    out << milliamps << " mA";
    return out.str();
}

}  // namespace

std::string PowerBudgetRule::description() const {
    return "The summed maximum current of all components must fit within the project power budget.";
}

std::vector<Finding> PowerBudgetRule::evaluate(const EvaluationContext& context) const {
    const Project& project = context.project();
    const std::optional<double>& budget = project.constraints().powerBudgetMa;
    if (!budget) return {};

    std::vector<Finding> findings;
    SpecGate totals(id());
    double knownMa = 0.0;
    std::vector<std::string> involved;
    std::vector<std::string> unknown;

    for (const Component& component : project.components()) {
        involved.push_back(component.id());
        SpecGate gate(id());
        const double* current = gate.require(component.specs().currentMa, component.id(), "maximum current");
        if (current && *current < 0.0) {
            findings.push_back(Finding{Severity::Warning, id(), {component.id()},
                                       component.id() + ": maximum current is negative; manual review required"});
            unknown.push_back(component.id());
            continue;
        }
        if (gate.blocked()) {
            findings.insert(findings.end(), gate.warnings().begin(), gate.warnings().end());
            unknown.push_back(component.id());
            continue;
        }
        totals.require(component.specs().currentMa, component.id(), "maximum current");
        knownMa += *current;
    }

    const std::string limit = formatMilliamps(*budget);
    Finding total;
    if (knownMa > *budget) {
        total = totals.verdict(Severity::Conflict, involved,
                               "Project: known current draw of " + formatMilliamps(knownMa) + " exceeds the " + limit +
                                   " power budget");
    } else if (!unknown.empty()) {
        total = Finding{Severity::Warning, id(), involved,
                        "Project: known allocations total " + formatMilliamps(knownMa) + " plus unknown draw from " +
                            join(unknown) + "; cannot confirm the " + limit + " power budget"};
    } else {
        total = totals.verdict(Severity::Pass, involved,
                               "Project: total current draw of " + formatMilliamps(knownMa) + " is within the " +
                                   limit + " power budget");
    }
    total.metrics = {{"knownMa", knownMa},
                     {"budgetMa", *budget},
                     {"unknownParts", static_cast<double>(unknown.size())}};
    findings.push_back(std::move(total));
    return findings;
}

}  // namespace embedded_market
