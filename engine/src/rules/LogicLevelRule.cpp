#include "embedded_market/rules/LogicLevelRule.hpp"

#include <utility>

#include "embedded_market/rules/SpecGate.hpp"
#include "embedded_market/util/Format.hpp"

namespace embedded_market {

std::string LogicLevelRule::description() const {
    return "Each side's logic-high output must be tolerated and recognized by the other side's inputs.";
}

std::vector<Finding> LogicLevelRule::evaluate(const Connection& connection,
                                              const Component& upstream,
                                              const Component& downstream,
                                              const EvaluationContext& context) const {
    const double tolerance = context.settings().voltageTolerance;
    std::vector<Finding> findings;

    const std::pair<const Component*, const Component*> directions[] = {{&upstream, &downstream},
                                                                       {&downstream, &upstream}};
    for (const auto& [driver, receiver] : directions) {
        SpecGate gate(id());
        const double* outputHigh = gate.require(driver->specs().logicOutputHigh, driver->id(), "logic output-high voltage");
        const double* inputMax = gate.require(receiver->specs().logicInputMax, receiver->id(), "maximum input voltage");
        const double* inputHighMin =
            gate.require(receiver->specs().logicInputHighMin, receiver->id(), "minimum input-high voltage");
        if (gate.blocked()) {
            findings.insert(findings.end(), gate.warnings().begin(), gate.warnings().end());
            continue;
        }

        const std::string prefix = connection.label() + ": " + driver->id() + " drives logic high at " +
                                   formatVolts(*outputHigh) + ", ";
        const std::vector<std::string> involved{driver->id(), receiver->id()};
        bool conflict = false;
        if (*outputHigh > *inputMax + tolerance) {
            findings.push_back(gate.verdict(Severity::Conflict, involved,
                                            prefix + "above " + receiver->id() + "'s maximum input voltage of " +
                                                formatVolts(*inputMax) + " (risk of damage)"));
            conflict = true;
        }
        if (*outputHigh < *inputHighMin - tolerance) {
            findings.push_back(gate.verdict(Severity::Conflict, involved,
                                            prefix + "below " + receiver->id() + "'s minimum input-high voltage of " +
                                                formatVolts(*inputHighMin) + " (highs may not register)"));
            conflict = true;
        }
        if (!conflict) {
            findings.push_back(gate.verdict(Severity::Pass, involved,
                                            prefix + "within " + receiver->id() + "'s input range [" +
                                                formatVolts(*inputHighMin) + ", " + formatVolts(*inputMax) + "]"));
        }
    }
    return findings;
}

}  // namespace embedded_market
