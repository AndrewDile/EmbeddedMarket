#pragma once

#include <iosfwd>
#include <map>
#include <string>
#include <vector>

namespace embedded_market {

// Ordered from least to most severe; a report's overall status is its most severe finding.
enum class Severity {
    Pass = 0,      // Check satisfied.
    Warning = 1,   // Check could not be trusted (missing, contradictory, unverified, or implausible data).
    Conflict = 2,  // Check failed.
};

std::string toString(Severity severity);
std::ostream& operator<<(std::ostream& os, Severity severity);

// One result from one rule, with an explanation a user can act on.
struct Finding {
    Severity severity = Severity::Pass;
    std::string ruleId;
    std::vector<std::string> componentIds;  // Components this finding is about.
    std::string message;
    std::map<std::string, double> metrics = {};  // Numbers behind the message, for display.

    bool involves(const std::string& componentId) const;
};

}  // namespace embedded_market
