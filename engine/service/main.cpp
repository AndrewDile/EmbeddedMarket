// HTTP front end for the compatibility engine. It holds no state: each request carries the whole
// project, so identical requests always produce identical reports (NFR-01).
#include <httplib.h>

#include <cstdlib>
#include <exception>
#include <iostream>
#include <nlohmann/json.hpp>

#include "JsonCodec.hpp"
#include "embedded_market/engine/CompatibilityEngine.hpp"

using nlohmann::json;

int main() {
    const char* portSetting = std::getenv("ENGINE_PORT");
    const int port = portSetting ? std::atoi(portSetting) : 8081;

    const embedded_market::CompatibilityEngine engine;
    httplib::Server server;

    server.Get("/health", [](const httplib::Request&, httplib::Response& response) {
        response.set_content(json{{"status", "ok"}}.dump(), "application/json");
    });

    server.Post("/evaluate", [&engine](const httplib::Request& request, httplib::Response& response) {
        try {
            const auto project = embedded_market::service::projectFromJson(json::parse(request.body));
            const auto report = engine.evaluate(project);
            response.set_content(embedded_market::service::reportToJson(report).dump(), "application/json");
        } catch (const std::exception& error) {
            response.status = 400;
            response.set_content(json{{"error", error.what()}}.dump(), "application/json");
        }
    });

    std::cout << "Rules engine listening on port " << port << std::endl;
    return server.listen("0.0.0.0", port) ? EXIT_SUCCESS : EXIT_FAILURE;
}
