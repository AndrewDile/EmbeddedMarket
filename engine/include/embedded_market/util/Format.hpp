#pragma once

#include <string>
#include <vector>

namespace embedded_market {

std::string formatVolts(double volts);               // e.g. "3.3 V"
std::string formatVoltageRange(double min, double max);  // e.g. "[1.71 V, 3.6 V]"
std::string join(const std::vector<std::string>& parts, const std::string& separator = ", ");

}  // namespace embedded_market
