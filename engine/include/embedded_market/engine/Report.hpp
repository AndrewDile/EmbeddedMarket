#pragma once

#include <string>
#include <vector>

#include "embedded_market/engine/Finding.hpp"

namespace embedded_market {

// All findings from one engine run. The overall status is the most severe finding
// (Conflict > Warning > Pass); an empty report passes.
class Report {
public:
    void add(Finding finding);

    Severity status() const;
    const std::vector<Finding>& findings() const { return findings_; }

    std::vector<Finding> withSeverity(Severity severity) const;
    std::vector<Finding> forComponent(const std::string& componentId) const;
    std::vector<Finding> fromRule(const std::string& ruleId) const;

private:
    std::vector<Finding> findings_;
};

}  // namespace embedded_market
