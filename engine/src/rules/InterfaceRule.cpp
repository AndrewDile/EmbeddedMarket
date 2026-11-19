#include "embedded_market/rules/InterfaceRule.hpp"

#include <utility>

#include "embedded_market/rules/SpecGate.hpp"

namespace embedded_market {

std::string InterfaceRule::description() const {
    return "Both ends of a connection must support the connection's chosen interface.";
}

std::vector<Finding> InterfaceRule::evaluate(const Connection& connection,
                                             const Component& upstream,
                                             const Component& downstream,
                                             const EvaluationContext&) const {
    SpecGate gate(id());
    const InterfaceSet* upstreamInterfaces = gate.require(upstream.specs().interfaces, upstream.id(), "supported interfaces");
    const InterfaceSet* downstreamInterfaces =
        gate.require(downstream.specs().interfaces, downstream.id(), "supported interfaces");
    if (gate.blocked()) return gate.warnings();

    const Interface chosen = connection.interface();
    std::vector<Finding> findings;
    const std::pair<const Component*, const InterfaceSet*> ends[] = {{&upstream, upstreamInterfaces},
                                                                     {&downstream, downstreamInterfaces}};
    for (const auto& [component, interfaces] : ends) {
        if (!interfaces->count(chosen)) {
            findings.push_back(gate.verdict(Severity::Conflict, {component->id()},
                                            connection.label() + ": " + component->id() + " does not support " +
                                                toString(chosen) + " (supports: " + toString(*interfaces) + ")"));
        }
    }
    if (findings.empty()) {
        findings.push_back(gate.verdict(Severity::Pass, {upstream.id(), downstream.id()},
                                        connection.label() + ": both ends support " + toString(chosen)));
    }
    return findings;
}

}  // namespace embedded_market
