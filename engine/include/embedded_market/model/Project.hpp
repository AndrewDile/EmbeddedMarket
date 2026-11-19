#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "embedded_market/model/Component.hpp"
#include "embedded_market/model/Connection.hpp"
#include "embedded_market/model/SpecValue.hpp"
#include "embedded_market/model/VoltageRange.hpp"

namespace embedded_market {

// User-declared project-wide limits. A limit left unset is not checked.
struct ProjectConstraints {
    std::optional<double> powerBudgetMa;
    std::optional<InterfaceSet> allowedInterfaces;
    std::optional<double> minClockMhz;
    std::optional<double> minFlashKb;
    std::optional<double> minRamKb;
};

// A project: the user-declared supply range and constraints plus a graph of components (nodes)
// and connections (edges).
class Project {
public:
    explicit Project(SpecValue<VoltageRange> supply = {}, ProjectConstraints constraints = {});

    // Throws std::invalid_argument if a component with the same id already exists.
    void addComponent(Component component);

    // Throws std::invalid_argument if either endpoint is unknown or both endpoints are the same.
    void addConnection(Connection connection);

    const SpecValue<VoltageRange>& supply() const { return supply_; }
    const ProjectConstraints& constraints() const { return constraints_; }
    const std::vector<Component>& components() const { return components_; }
    const std::vector<Connection>& connections() const { return connections_; }

    // Returns nullptr if no component has the id. The pointer is invalidated by addComponent.
    const Component* findComponent(const std::string& id) const;

    // Connections where the component is the upstream / downstream side.
    std::vector<const Connection*> downstreamOf(const std::string& id) const;
    std::vector<const Connection*> upstreamOf(const std::string& id) const;

private:
    SpecValue<VoltageRange> supply_;
    ProjectConstraints constraints_;
    std::vector<Component> components_;
    std::unordered_map<std::string, std::size_t> indexById_;
    std::vector<Connection> connections_;
};

}  // namespace embedded_market
