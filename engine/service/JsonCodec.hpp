#pragma once

#include <nlohmann/json.hpp>

#include "embedded_market/engine/Report.hpp"
#include "embedded_market/model/Project.hpp"

namespace embedded_market::service {

// Builds a Project from the /evaluate request body.
// Throws nlohmann::json::exception or std::invalid_argument on a malformed request.
Project projectFromJson(const nlohmann::json& body);

nlohmann::json reportToJson(const Report& report);

}  // namespace embedded_market::service
