#pragma once

#include <set>
#include <string>

namespace embedded_market {

// Communication interfaces a component can expose on a connection.
enum class Interface {
    I2C,
    SPI,
    UART,
    GPIO,  // Discrete digital signals (enable, step/dir, LEDs); pins come from the part's extra pins.
};

using InterfaceSet = std::set<Interface>;

std::string toString(Interface iface);
std::string toString(const InterfaceSet& interfaces);  // e.g. "I2C, SPI" or "none"

// Host pins an interface consumes.
struct InterfacePins {
    int sharedPerHost;  // Bus signals used once per host, however many devices share them.
    int perDevice;      // Signals each attached device needs on its own.
};

// I2C: SDA/SCL shared. SPI: SCK/MOSI/MISO shared plus one chip-select per device.
// UART: TX/RX per device. GPIO: none beyond the part's extra pins.
InterfacePins interfacePins(Interface iface);

}  // namespace embedded_market
