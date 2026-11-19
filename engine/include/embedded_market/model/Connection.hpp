#pragma once

#include <string>

#include "embedded_market/model/Interface.hpp"

namespace embedded_market {

// A link between two components over one interface.
//
// The upstream side hosts the link (an MCU, an I2C multiplexer, a motor driver) and the
// downstream side is attached to it. Either side may be any kind of component, so
// peripheral-to-peripheral chains (MCU -> mux -> sensor, MCU -> driver -> motor) are just
// more connections in the project graph.
class Connection {
public:
    Connection(std::string upstreamId, std::string downstreamId, Interface iface);

    const std::string& upstream() const { return upstream_; }
    const std::string& downstream() const { return downstream_; }
    Interface interface() const { return interface_; }

    std::string label() const;  // e.g. "MCU1 -> U2 (I2C)"

private:
    std::string upstream_;
    std::string downstream_;
    Interface interface_;
};

}  // namespace embedded_market
