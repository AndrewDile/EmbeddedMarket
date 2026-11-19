#include "embedded_market/engine/Report.hpp"

#include <algorithm>
#include <iterator>
#include <ostream>
#include <utility>

namespace embedded_market {

std::string toString(Severity severity) {
    switch (severity) {
        case Severity::Pass: return "Pass";
        case Severity::Warning: return "Warning";
        case Severity::Conflict: return "Conflict";
    }
    return "Unknown";
}

std::ostream& operator<<(std::ostream& os, Severity severity) { return os << toString(severity); }

bool Finding::involves(const std::string& componentId) const {
    return std::find(componentIds.begin(), componentIds.end(), componentId) != componentIds.end();
}

void Report::add(Finding finding) { findings_.push_back(std::move(finding)); }

Severity Report::status() const {
    Severity worst = Severity::Pass;
    for (const Finding& finding : findings_) {
        worst = std::max(worst, finding.severity);
    }
    return worst;
}

std::vector<Finding> Report::withSeverity(Severity severity) const {
    std::vector<Finding> out;
    std::copy_if(findings_.begin(), findings_.end(), std::back_inserter(out),
                 [&](const Finding& f) { return f.severity == severity; });
    return out;
}

std::vector<Finding> Report::forComponent(const std::string& componentId) const {
    std::vector<Finding> out;
    std::copy_if(findings_.begin(), findings_.end(), std::back_inserter(out),
                 [&](const Finding& f) { return f.involves(componentId); });
    return out;
}

std::vector<Finding> Report::fromRule(const std::string& ruleId) const {
    std::vector<Finding> out;
    std::copy_if(findings_.begin(), findings_.end(), std::back_inserter(out),
                 [&](const Finding& f) { return f.ruleId == ruleId; });
    return out;
}

}  // namespace embedded_market
