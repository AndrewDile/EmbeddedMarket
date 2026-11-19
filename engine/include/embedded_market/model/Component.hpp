#pragma once

#include <string>

#include "embedded_market/model/Interface.hpp"
#include "embedded_market/model/SpecValue.hpp"

namespace embedded_market {

enum class ComponentRole {
    Controller,  // MCU, SoC, or other part that hosts connections.
    Peripheral,  // Sensor, driver board, actuator, multiplexer, etc.
};

std::string toString(ComponentRole role);

// Normalized specifications for a component. Every field is a SpecValue so rules can tell
// verified data from unverified, missing, or contradictory data.
struct ComponentSpecs {
    // Operating supply voltage range, in volts.
    SpecValue<double> supplyVoltageMin;
    SpecValue<double> supplyVoltageMax;

    SpecValue<InterfaceSet> interfaces;

    // Logic levels, in volts.
    SpecValue<double> logicOutputHigh;    // V_OH: voltage this part drives for a logic high.
    SpecValue<double> logicInputHighMin;  // V_IH(min): lowest voltage this part reads as a logic high.
    SpecValue<double> logicInputMax;      // Highest voltage this part's inputs tolerate.

    // Pins.
    SpecValue<int> availableGpio;  // Pins this part can offer to its downstream parts.
    SpecValue<int> extraPins;      // Dedicated pins this part needs from its upstream host beyond the
                                   // interface signals (interrupt, reset, enable). Zero if none.

    SpecValue<double> currentMa;  // Maximum current draw, in milliamps.

    // Controller capabilities.
    SpecValue<double> clockMhz;
    SpecValue<double> flashKb;
    SpecValue<double> ramKb;
};

class Component {
public:
    Component(std::string id, ComponentRole role, ComponentSpecs specs = {});

    const std::string& id() const { return id_; }
    ComponentRole role() const { return role_; }

    const ComponentSpecs& specs() const { return specs_; }
    ComponentSpecs& specs() { return specs_; }

private:
    std::string id_;
    ComponentRole role_;
    ComponentSpecs specs_;
};

}  // namespace embedded_market
