#include "embedded_market/rules/ProjectConstraintsRule.hpp"

#include <sstream>

#include "embedded_market/rules/SpecGate.hpp"
#include "embedded_market/util/Format.hpp"

namespace embedded_market {

namespace {

std::string formatNumber(double value) {
    std::ostringstream out;
    out << value;
    return out.str();
}

struct ControllerMinimum {
    const char* name;
    const char* unit;
    const SpecValue<double>& spec;
    const std::optional<double>& minimum;
};

}  // namespace

std::string ProjectConstraintsRule::description() const {
    return "Controllers must meet the project's clock and memory minimums, and connections must use allowed interfaces.";
}

std::vector<Finding> ProjectConstraintsRule::evaluate(const EvaluationContext& context) const {
    const Project& project = context.project();
    const ProjectConstraints& constraints = project.constraints();
    std::vector<Finding> findings;

    for (const Component& component : project.components()) {
        if (component.role() != ComponentRole::Controller) continue;
        const ComponentSpecs& specs = component.specs();
        const ControllerMinimum minimums[] = {
            {"clock speed", "MHz", specs.clockMhz, constraints.minClockMhz},
            {"flash memory", "KB", specs.flashKb, constraints.minFlashKb},
            {"RAM", "KB", specs.ramKb, constraints.minRamKb},
        };
        for (const ControllerMinimum& check : minimums) {
            if (!check.minimum) continue;
            SpecGate gate(id());
            const double* value = gate.require(check.spec, component.id(), check.name);
            if (gate.blocked()) {
                findings.insert(findings.end(), gate.warnings().begin(), gate.warnings().end());
                continue;
            }
            const std::string summary = component.id() + ": " + check.name + " of " + formatNumber(*value) + " " +
                                        check.unit;
            const std::string required = formatNumber(*check.minimum) + " " + check.unit;
            if (*value < *check.minimum) {
                findings.push_back(gate.verdict(Severity::Conflict, {component.id()},
                                                summary + " is below the project minimum of " + required));
            } else {
                findings.push_back(gate.verdict(Severity::Pass, {component.id()},
                                                summary + " meets the project minimum of " + required));
            }
        }
    }

    if (constraints.allowedInterfaces) {
        const InterfaceSet& allowed = *constraints.allowedInterfaces;
        for (const Connection& connection : project.connections()) {
            const std::vector<std::string> involved{connection.upstream(), connection.downstream()};
            if (allowed.count(connection.interface())) {
                findings.push_back(Finding{Severity::Pass, id(), involved,
                                           connection.label() + ": interface is allowed by the project"});
            } else {
                findings.push_back(Finding{Severity::Conflict, id(), involved,
                                           connection.label() + ": interface is not allowed by the project (allowed: " +
                                               toString(allowed) + ")"});
            }
        }
    }
    return findings;
}

}  // namespace embedded_market
