"""Translates between database records, the rules engine, and the UI's check response."""
import os

import requests

ENGINE_TIMEOUT_SECONDS = 5
DISPLAY_REF = "U3"
LED_CIRCUIT_REF = "C1"

SPEC_COLUMNS = {
    "supplyVoltageMin": "supply_min_v",
    "supplyVoltageMax": "supply_max_v",
    "logicOutputHigh": "logic_output_high_v",
    "logicInputHighMin": "logic_input_high_min_v",
    "logicInputMax": "logic_input_max_v",
    "currentMa": "current_ma",
    "clockMhz": "clock_mhz",
    "flashKb": "flash_kb",
    "ramKb": "ram_kb",
}
COUNT_COLUMNS = {"availableGpio": "available_gpio", "extraPins": "extra_pins"}


def engine_url():
    return os.environ.get("ENGINE_URL", "http://engine:8081")


def ping_engine():
    requests.get(f"{engine_url()}/health", timeout=ENGINE_TIMEOUT_SECONDS).raise_for_status()


def evaluate(project):
    response = requests.post(f"{engine_url()}/evaluate", json=project, timeout=ENGINE_TIMEOUT_SECONDS)
    response.raise_for_status()
    return response.json()


def _verified(value):
    return {"value": value, "status": "verified"}


def _interfaces(value):
    # PyMySQL can return a SET column as a Python set or a comma-separated string.
    names = value.split(",") if isinstance(value, str) else value
    return sorted(names)


def _specs(part, current_ma):
    specs = {name: _verified(float(part[column]))
             for name, column in SPEC_COLUMNS.items() if part[column] is not None}
    specs.update({name: _verified(int(part[column]))
                  for name, column in COUNT_COLUMNS.items() if part[column] is not None})
    if part["interfaces"]:
        specs["interfaces"] = _verified(_interfaces(part["interfaces"]))
    if current_ma is not None:
        specs["currentMa"] = _verified(float(current_ma))
    return specs


def build_project(bom, display, led_current):
    """Builds the engine request for the BOM with the user's display and LED current applied."""
    components = []
    connections = []
    for item in bom["items"]:
        ref = item["ref"]
        part = bom["parts"][display if ref == DISPLAY_REF else item["part_number"]]
        current = led_current if ref == LED_CIRCUIT_REF else None
        components.append({"id": ref, "role": part["role"], "specs": _specs(part, current)})
        if item["upstream_ref"]:
            connections.append({"upstream": item["upstream_ref"], "downstream": ref,
                                "interface": item["interface"]})
    return {
        "supply": {"min": float(bom["supply_min_v"]), "max": float(bom["supply_max_v"])},
        "constraints": {
            "powerBudgetMa": float(bom["power_budget_ma"]),
            "allowedInterfaces": _interfaces(bom["allowed_interfaces"]),
            "minClockMhz": float(bom["min_clock_mhz"]),
            "minFlashKb": float(bom["min_flash_kb"]),
            "minRamKb": float(bom["min_ram_kb"]),
        },
        "components": components,
        "connections": connections,
    }


def summarize(report):
    """Reduces an engine report to the response the UI's checkCompatibility() validates."""
    findings = report["findings"]
    power = next((f for f in findings if f["ruleId"] == "power-budget" and "knownMa" in f["metrics"]), None)
    if power is None:
        raise ValueError("Engine report has no power-budget total")

    return {
        "conflict": any(f["severity"] == "conflict" and f["ruleId"] != "power-budget" for f in findings),
        "known": power["metrics"]["unknownParts"] == 0,
        "total": power["metrics"]["knownMa"],
        "overBudget": power["severity"] == "conflict",
        "ready": report["status"] == "pass",
        "checks": report["checks"],
        "findings": findings,
    }
