#include "embedded_market/model/Project.hpp"

#include <stdexcept>
#include <utility>

namespace embedded_market {

Project::Project(SpecValue<VoltageRange> supply, ProjectConstraints constraints)
    : supply_(std::move(supply)), constraints_(std::move(constraints)) {}

void Project::addComponent(Component component) {
    if (indexById_.count(component.id())) {
        throw std::invalid_argument("Duplicate component id: " + component.id());
    }
    indexById_.emplace(component.id(), components_.size());
    components_.push_back(std::move(component));
}

void Project::addConnection(Connection connection) {
    for (const std::string* id : {&connection.upstream(), &connection.downstream()}) {
        if (!indexById_.count(*id)) {
            throw std::invalid_argument("Connection references unknown component: " + *id);
        }
    }
    if (connection.upstream() == connection.downstream()) {
        throw std::invalid_argument("Component cannot connect to itself: " + connection.upstream());
    }
    connections_.push_back(std::move(connection));
}

const Component* Project::findComponent(const std::string& id) const {
    auto it = indexById_.find(id);
    return it == indexById_.end() ? nullptr : &components_[it->second];
}

std::vector<const Connection*> Project::downstreamOf(const std::string& id) const {
    std::vector<const Connection*> out;
    for (const Connection& connection : connections_) {
        if (connection.upstream() == id) out.push_back(&connection);
    }
    return out;
}

std::vector<const Connection*> Project::upstreamOf(const std::string& id) const {
    std::vector<const Connection*> out;
    for (const Connection& connection : connections_) {
        if (connection.downstream() == id) out.push_back(&connection);
    }
    return out;
}

}  // namespace embedded_market
