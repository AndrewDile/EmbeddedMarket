#include <stdexcept>
#include <utility>

#include "embedded_market/model/Component.hpp"
#include "embedded_market/model/Connection.hpp"
#include "embedded_market/model/Interface.hpp"
#include "embedded_market/model/SpecValue.hpp"

namespace embedded_market {

std::string toString(DataStatus status) {
    switch (status) {
        case DataStatus::Verified: return "Verified";
        case DataStatus::Unverified: return "Unverified";
        case DataStatus::Missing: return "Missing";
        case DataStatus::Contradictory: return "Contradictory";
    }
    return "Unknown";
}

std::string toString(Interface iface) {
    switch (iface) {
        case Interface::I2C: return "I2C";
        case Interface::SPI: return "SPI";
        case Interface::UART: return "UART";
        case Interface::GPIO: return "GPIO";
    }
    return "Unknown";
}

InterfacePins interfacePins(Interface iface) {
    switch (iface) {
        case Interface::I2C: return {2, 0};
        case Interface::SPI: return {3, 1};
        case Interface::UART: return {0, 2};
        case Interface::GPIO: return {0, 0};
    }
    return {0, 0};
}

std::string toString(const InterfaceSet& interfaces) {
    if (interfaces.empty()) return "none";
    std::string out;
    for (Interface iface : interfaces) {
        if (!out.empty()) out += ", ";
        out += toString(iface);
    }
    return out;
}

std::string toString(ComponentRole role) {
    switch (role) {
        case ComponentRole::Controller: return "Controller";
        case ComponentRole::Peripheral: return "Peripheral";
    }
    return "Unknown";
}

Component::Component(std::string id, ComponentRole role, ComponentSpecs specs)
    : id_(std::move(id)), role_(role), specs_(std::move(specs)) {
    if (id_.empty()) throw std::invalid_argument("Component id must not be empty");
}

Connection::Connection(std::string upstreamId, std::string downstreamId, Interface iface)
    : upstream_(std::move(upstreamId)), downstream_(std::move(downstreamId)), interface_(iface) {}

std::string Connection::label() const {
    return upstream_ + " -> " + downstream_ + " (" + toString(interface_) + ")";
}

}  // namespace embedded_market
