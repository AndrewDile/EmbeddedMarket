#include "embedded_market/rules/SupplyVoltageRule.hpp"

#include "embedded_market/rules/SpecGate.hpp"
#include "embedded_market/util/Format.hpp"

namespace embedded_market {

std::string SupplyVoltageRule::description() const {
    return "The entire project supply range must fall within each component's operating voltage range.";
}

std::vector<Finding> SupplyVoltageRule::evaluate(const Component& component,
                                                 const EvaluationContext& context) const {
    const std::string& id = component.id();
    SpecGate gate(this->id());
    const VoltageRange* supply = gate.require(context.project().supply(), id, "project supply range");
    const double* min = gate.require(component.specs().supplyVoltageMin, id, "minimum supply voltage");
    const double* max = gate.require(component.specs().supplyVoltageMax, id, "maximum supply voltage");
    if (gate.blocked()) return gate.warnings();

    const double tolerance = context.settings().voltageTolerance;
    const std::string operating = formatVoltageRange(*min, *max);

    if (*min > *max + tolerance) {
        return {Finding{Severity::Warning, this->id(), {id},
                        id + ": operating range " + operating + " is inverted; manual review required"}};
    }

    std::vector<std::string> problems;
    if (supply->min < *min - tolerance) {
        problems.push_back("drops to " + formatVolts(supply->min) + ", below its minimum operating voltage of " +
                           formatVolts(*min));
    }
    if (supply->max > *max + tolerance) {
        problems.push_back("reaches " + formatVolts(supply->max) + ", above its maximum operating voltage of " +
                           formatVolts(*max));
    }

    const std::string prefix = id + ": supply range " + formatVoltageRange(supply->min, supply->max) + " ";
    if (!problems.empty()) {
        return {gate.verdict(Severity::Conflict, {id}, prefix + join(problems, " and "))};
    }
    return {gate.verdict(Severity::Pass, {id}, prefix + "is within its operating range " + operating)};
}

}  // namespace embedded_market
