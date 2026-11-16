#include "embedded_market/util/Format.hpp"

#include <sstream>

namespace embedded_market {

std::string formatVolts(double volts) {
    std::ostringstream out;
    out.precision(6);
    out << volts << " V";
    return out.str();
}

std::string formatVoltageRange(double min, double max) {
    return "[" + formatVolts(min) + ", " + formatVolts(max) + "]";
}

std::string join(const std::vector<std::string>& parts, const std::string& separator) {
    std::string out;
    for (const std::string& part : parts) {
        if (!out.empty()) out += separator;
        out += part;
    }
    return out;
}

}  // namespace embedded_market
