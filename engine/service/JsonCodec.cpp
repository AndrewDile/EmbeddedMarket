#include "JsonCodec.hpp"

#include <optional>
#include <stdexcept>
#include <string>

using nlohmann::json;

namespace embedded_market::service {

namespace {

Interface interfaceFromString(const std::string& name) {
    if (name == "I2C") return Interface::I2C;
    if (name == "SPI") return Interface::SPI;
    if (name == "UART") return Interface::UART;
    if (name == "GPIO") return Interface::GPIO;
    throw std::invalid_argument("Unknown interface: " + name);
}

InterfaceSet interfaceSetFromJson(const json& names) {
    InterfaceSet interfaces;
    for (const json& name : names) interfaces.insert(interfaceFromString(name.get<std::string>()));
    return interfaces;
}

// A spec is {"value": ..., "status": "verified" | "unverified" | "missing" | "contradictory"}.
// An absent spec is Missing.
template <typename T, typename Parse>
SpecValue<T> specFromJson(const json& specs, const char* field, Parse parse) {
    if (!specs.contains(field)) return SpecValue<T>::missing();
    const json& spec = specs.at(field);
    const std::string status = spec.at("status").get<std::string>();
    if (status == "missing") return SpecValue<T>::missing();
    if (status == "contradictory") return SpecValue<T>::contradictory();
    if (status == "verified") return SpecValue<T>::verified(parse(spec.at("value")));
    if (status == "unverified") return SpecValue<T>::unverified(parse(spec.at("value")));
    throw std::invalid_argument("Unknown data status: " + status);
}

SpecValue<double> number(const json& specs, const char* field) {
    return specFromJson<double>(specs, field, [](const json& v) { return v.get<double>(); });
}

SpecValue<int> count(const json& specs, const char* field) {
    return specFromJson<int>(specs, field, [](const json& v) { return v.get<int>(); });
}

std::optional<double> optionalNumber(const json& object, const char* field) {
    if (!object.contains(field) || object.at(field).is_null()) return std::nullopt;
    return object.at(field).get<double>();
}

ComponentRole roleFromString(const std::string& role) {
    if (role == "controller") return ComponentRole::Controller;
    if (role == "peripheral") return ComponentRole::Peripheral;
    throw std::invalid_argument("Unknown component role: " + role);
}

Component componentFromJson(const json& body) {
    const json& specs = body.at("specs");
    ComponentSpecs out;
    out.supplyVoltageMin = number(specs, "supplyVoltageMin");
    out.supplyVoltageMax = number(specs, "supplyVoltageMax");
    out.interfaces = specFromJson<InterfaceSet>(specs, "interfaces", interfaceSetFromJson);
    out.logicOutputHigh = number(specs, "logicOutputHigh");
    out.logicInputHighMin = number(specs, "logicInputHighMin");
    out.logicInputMax = number(specs, "logicInputMax");
    out.availableGpio = count(specs, "availableGpio");
    out.extraPins = count(specs, "extraPins");
    out.currentMa = number(specs, "currentMa");
    out.clockMhz = number(specs, "clockMhz");
    out.flashKb = number(specs, "flashKb");
    out.ramKb = number(specs, "ramKb");
    return Component(body.at("id").get<std::string>(), roleFromString(body.at("role").get<std::string>()), out);
}

ProjectConstraints constraintsFromJson(const json& body) {
    ProjectConstraints constraints;
    constraints.powerBudgetMa = optionalNumber(body, "powerBudgetMa");
    constraints.minClockMhz = optionalNumber(body, "minClockMhz");
    constraints.minFlashKb = optionalNumber(body, "minFlashKb");
    constraints.minRamKb = optionalNumber(body, "minRamKb");
    if (body.contains("allowedInterfaces") && !body.at("allowedInterfaces").is_null()) {
        constraints.allowedInterfaces = interfaceSetFromJson(body.at("allowedInterfaces"));
    }
    return constraints;
}

std::string severityName(Severity severity) {
    switch (severity) {
        case Severity::Pass: return "pass";
        case Severity::Warning: return "warning";
        case Severity::Conflict: return "conflict";
    }
    return "warning";
}

}  // namespace

Project projectFromJson(const json& body) {
    const json& supply = body.at("supply");
    Project project(SpecValue<VoltageRange>::verified({supply.at("min").get<double>(), supply.at("max").get<double>()}),
                    constraintsFromJson(body.value("constraints", json::object())));
    for (const json& component : body.at("components")) project.addComponent(componentFromJson(component));
    for (const json& connection : body.at("connections")) {
        project.addConnection(Connection(connection.at("upstream").get<std::string>(),
                                         connection.at("downstream").get<std::string>(),
                                         interfaceFromString(connection.at("interface").get<std::string>())));
    }
    return project;
}

json reportToJson(const Report& report) {
    json findings = json::array();
    for (const Finding& finding : report.findings()) {
        findings.push_back({{"severity", severityName(finding.severity)},
                            {"ruleId", finding.ruleId},
                            {"componentIds", finding.componentIds},
                            {"message", finding.message},
                            {"metrics", finding.metrics}});
    }
    const json checks = {{"total", report.findings().size()},
                         {"passed", report.withSeverity(Severity::Pass).size()},
                         {"warnings", report.withSeverity(Severity::Warning).size()},
                         {"conflicts", report.withSeverity(Severity::Conflict).size()}};
    return {{"status", severityName(report.status())}, {"checks", checks}, {"findings", findings}};
}

}  // namespace embedded_market::service
