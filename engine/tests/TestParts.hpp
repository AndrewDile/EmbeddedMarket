#pragma once

#include <string>
#include <utility>

#include "embedded_market/model/Component.hpp"
#include "embedded_market/model/VoltageRange.hpp"

namespace em_test {

using namespace embedded_market;

// Verified project supply ranges.
inline SpecValue<VoltageRange> supply(double volts) { return SpecValue<VoltageRange>::verified(VoltageRange::fixed(volts)); }
inline SpecValue<VoltageRange> supply(double min, double max) { return SpecValue<VoltageRange>::verified({min, max}); }

// Fluent builder for test components. Every setter stores a Verified value; tests that need
// other statuses edit specs() directly.
class PartBuilder {
public:
    PartBuilder(std::string id, ComponentRole role) : id_(std::move(id)), role_(role) {}

    PartBuilder& supply(double min, double max) {
        specs_.supplyVoltageMin = SpecValue<double>::verified(min);
        specs_.supplyVoltageMax = SpecValue<double>::verified(max);
        return *this;
    }
    PartBuilder& interfaces(InterfaceSet set) {
        specs_.interfaces = SpecValue<InterfaceSet>::verified(std::move(set));
        return *this;
    }
    PartBuilder& logic(double outputHigh, double inputHighMin, double inputMax) {
        specs_.logicOutputHigh = SpecValue<double>::verified(outputHigh);
        specs_.logicInputHighMin = SpecValue<double>::verified(inputHighMin);
        specs_.logicInputMax = SpecValue<double>::verified(inputMax);
        return *this;
    }
    PartBuilder& gpio(int available) {
        specs_.availableGpio = SpecValue<int>::verified(available);
        return *this;
    }
    PartBuilder& extraPins(int extra) {
        specs_.extraPins = SpecValue<int>::verified(extra);
        return *this;
    }

    ComponentSpecs& specs() { return specs_; }
    Component build() const { return Component(id_, role_, specs_); }

private:
    std::string id_;
    ComponentRole role_;
    ComponentSpecs specs_;
};

// A typical 3.3 V microcontroller with I2C and SPI (the FR-03 controller), every spec verified.
inline PartBuilder mcu3v3(std::string id = "MCU1") {
    PartBuilder part(std::move(id), ComponentRole::Controller);
    part.supply(1.8, 3.6).interfaces({Interface::I2C, Interface::SPI}).logic(3.3, 2.0, 3.6).gpio(20);
    return part;
}

// A typical 3.3 V sensor with no extra pins, every spec verified.
inline PartBuilder sensor3v3(std::string id, InterfaceSet interfaces) {
    PartBuilder part(std::move(id), ComponentRole::Peripheral);
    part.supply(1.71, 3.6).interfaces(std::move(interfaces)).logic(3.3, 2.0, 3.6).extraPins(0);
    return part;
}

}  // namespace em_test
