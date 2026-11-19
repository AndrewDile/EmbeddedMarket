#pragma once

#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include "embedded_market/engine/Finding.hpp"
#include "embedded_market/model/SpecValue.hpp"
#include "embedded_market/model/VoltageRange.hpp"
#include "embedded_market/util/Format.hpp"

namespace embedded_market {

// Applies the engine's uncertainty policy for a rule:
//   - A Missing, Contradictory, or invalid input blocks the check and records a Warning that
//     names the component and field.
//   - An Unverified input still lets the check run, but a Pass is downgraded to a Warning, and
//     any verdict notes which inputs were unverified.
// Unverified data can therefore never produce a Pass.
//
// Usage: require() every input, return warnings() if blocked(), otherwise return verdict().
class SpecGate {
public:
    explicit SpecGate(std::string ruleId) : ruleId_(std::move(ruleId)) {}

    // Returns the value if usable, otherwise nullptr.
    template <typename T>
    const T* require(const SpecValue<T>& spec, const std::string& componentId, const std::string& field) {
        switch (spec.status()) {
            case DataStatus::Missing:
                block(componentId, field + " is missing; manual review required");
                return nullptr;
            case DataStatus::Contradictory:
                block(componentId, field + " has contradictory values across sources; manual review required");
                return nullptr;
            case DataStatus::Unverified:
                unverified_.push_back(componentId + " " + field);
                break;
            case DataStatus::Verified:
                break;
        }
        if (!isUsable(spec.value())) {
            block(componentId, field + " has an invalid value; manual review required");
            return nullptr;
        }
        return &spec.value();
    }

    bool blocked() const { return !warnings_.empty(); }
    const std::vector<Finding>& warnings() const { return warnings_; }

    Finding verdict(Severity severity, std::vector<std::string> componentIds, std::string message) const {
        if (!unverified_.empty()) {
            message += " (based on unverified data: " + join(unverified_) + ")";
            if (severity == Severity::Pass) {
                severity = Severity::Warning;
                message += "; manual review required";
            }
        }
        return Finding{severity, ruleId_, std::move(componentIds), std::move(message)};
    }

private:
    template <typename T>
    static bool isUsable(const T&) { return true; }
    static bool isUsable(double value) { return std::isfinite(value); }
    static bool isUsable(int count) { return count >= 0; }  // Integer specs are counts.
    static bool isUsable(const VoltageRange& range) {
        return std::isfinite(range.min) && std::isfinite(range.max) && range.min <= range.max;
    }

    void block(const std::string& componentId, const std::string& problem) {
        warnings_.push_back(Finding{Severity::Warning, ruleId_, {componentId}, componentId + ": " + problem});
    }

    std::string ruleId_;
    std::vector<Finding> warnings_;
    std::vector<std::string> unverified_;
};

}  // namespace embedded_market
