#pragma once

namespace embedded_market {

// An inclusive voltage range, in volts. A fixed supply is a range with min == max.
struct VoltageRange {
    double min = 0.0;
    double max = 0.0;

    static VoltageRange fixed(double volts) { return {volts, volts}; }
};

}  // namespace embedded_market
