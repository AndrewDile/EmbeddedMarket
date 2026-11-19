#include "embedded_market/rules/PinBudgetRule.hpp"

#include <set>
#include <utility>

#include "embedded_market/rules/SpecGate.hpp"
#include "embedded_market/util/Format.hpp"

namespace embedded_market {

std::string PinBudgetRule::description() const {
    return "Each host's connections must not need more pins than the host has available.";
}

std::vector<Finding> PinBudgetRule::evaluate(const EvaluationContext& context) const {
    const Project& project = context.project();
    std::vector<Finding> findings;

    for (const Component& host : project.components()) {
        const auto links = project.downstreamOf(host.id());
        if (links.empty()) continue;

        SpecGate gate(id());
        const int* available = gate.require(host.specs().availableGpio, host.id(), "available GPIO count");

        int used = 0;
        std::vector<std::string> breakdown;  // e.g. "I2C bus 2", "U1 SPI 1", "U1 extra 1"
        std::vector<std::string> involved{host.id()};
        std::set<Interface> buses;
        std::set<std::pair<std::string, Interface>> countedLinks;
        std::set<std::string> countedParts;

        for (const Connection* link : links) {
            const Interface iface = link->interface();
            const std::string& partId = link->downstream();
            if (!countedLinks.insert({partId, iface}).second) continue;

            const InterfacePins pins = interfacePins(iface);
            if (pins.sharedPerHost > 0 && buses.insert(iface).second) {
                used += pins.sharedPerHost;
                breakdown.push_back(toString(iface) + " bus " + std::to_string(pins.sharedPerHost));
            }
            if (pins.perDevice > 0) {
                used += pins.perDevice;
                breakdown.push_back(partId + " " + toString(iface) + " " + std::to_string(pins.perDevice));
            }
            if (countedParts.insert(partId).second) {
                involved.push_back(partId);
                const Component& part = *project.findComponent(partId);
                const int* extra = gate.require(part.specs().extraPins, partId, "extra pin count");
                if (extra && *extra > 0) {
                    used += *extra;
                    breakdown.push_back(partId + " extra " + std::to_string(*extra));
                }
            }
        }
        if (gate.blocked()) {
            findings.insert(findings.end(), gate.warnings().begin(), gate.warnings().end());
            continue;
        }

        const std::string need = host.id() + ": connections need " + std::to_string(used) + " pins (" +
                                 (breakdown.empty() ? std::string("none") : join(breakdown)) + ")";
        Finding finding = used > *available
                              ? gate.verdict(Severity::Conflict, involved,
                                             need + " but only " + std::to_string(*available) + " GPIO are available")
                              : gate.verdict(Severity::Pass, involved,
                                             need + ", within " + std::to_string(*available) + " available GPIO");
        finding.metrics = {{"pinsUsed", static_cast<double>(used)}, {"pinsAvailable", static_cast<double>(*available)}};
        findings.push_back(std::move(finding));
    }
    return findings;
}

}  // namespace embedded_market
