#include "embedded_market/rules/SpecSanityRule.hpp"

#include <cmath>

#include "embedded_market/util/Format.hpp"

namespace embedded_market {

namespace {

struct VoltageField {
    const char* name;
    const SpecValue<double>& spec;
};

struct CountField {
    const char* name;
    const SpecValue<int>& spec;
    int plausibleMax;
};

bool finiteValue(const SpecValue<double>& spec) { return spec.hasValue() && std::isfinite(spec.value()); }

}  // namespace

std::string SpecSanityRule::description() const {
    return "Specification values must be valid and physically plausible.";
}

std::vector<Finding> SpecSanityRule::evaluate(const Component& component, const EvaluationContext& context) const {
    const ComponentSpecs& specs = component.specs();
    const SanityLimits& limits = context.settings().limits;
    const double tolerance = context.settings().voltageTolerance;
    std::vector<std::string> problems;

    const VoltageField voltages[] = {
        {"minimum supply voltage", specs.supplyVoltageMin},
        {"maximum supply voltage", specs.supplyVoltageMax},
        {"logic output-high voltage", specs.logicOutputHigh},
        {"minimum input-high voltage", specs.logicInputHighMin},
        {"maximum input voltage", specs.logicInputMax},
    };
    for (const VoltageField& field : voltages) {
        if (!field.spec.hasValue()) continue;
        const double v = field.spec.value();
        if (!std::isfinite(v)) {
            problems.push_back(std::string(field.name) + " is not a finite number");
        } else if (v < 0.0) {
            problems.push_back(std::string(field.name) + " is negative (" + formatVolts(v) + ")");
        } else if (v > limits.maxPlausibleVoltage) {
            problems.push_back(std::string(field.name) + " of " + formatVolts(v) + " is implausibly high (limit " +
                               formatVolts(limits.maxPlausibleVoltage) + ")");
        }
    }

    // Pairs where the first value must not exceed the second.
    const std::pair<VoltageField, VoltageField> ordered[] = {
        {voltages[0], voltages[1]},  // supply min <= supply max
        {voltages[3], voltages[4]},  // input-high min <= input max
        {voltages[2], voltages[1]},  // output-high <= supply max
    };
    for (const auto& [low, high] : ordered) {
        if (finiteValue(low.spec) && finiteValue(high.spec) && low.spec.value() > high.spec.value() + tolerance) {
            problems.push_back(std::string(low.name) + " (" + formatVolts(low.spec.value()) + ") exceeds " +
                               high.name + " (" + formatVolts(high.spec.value()) + ")");
        }
    }

    const CountField counts[] = {
        {"available GPIO count", specs.availableGpio, limits.maxPlausibleGpio},
        {"extra pin count", specs.extraPins, limits.maxPlausibleExtraPins},
    };
    for (const CountField& field : counts) {
        if (!field.spec.hasValue()) continue;
        const int n = field.spec.value();
        if (n < 0) {
            problems.push_back(std::string(field.name) + " is negative (" + std::to_string(n) + ")");
        } else if (n > field.plausibleMax) {
            problems.push_back(std::string(field.name) + " of " + std::to_string(n) + " is implausibly high (limit " +
                               std::to_string(field.plausibleMax) + ")");
        }
    }

    const std::string& id = component.id();
    if (problems.empty()) {
        return {Finding{Severity::Pass, this->id(), {id}, id + ": specification values are plausible"}};
    }
    std::vector<Finding> findings;
    for (const std::string& problem : problems) {
        findings.push_back(Finding{Severity::Warning, this->id(), {id}, id + ": " + problem + "; manual review required"});
    }
    return findings;
}

}  // namespace embedded_market
