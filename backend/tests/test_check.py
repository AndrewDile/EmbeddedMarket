import sys
import unittest
from pathlib import Path
from unittest.mock import patch

import pymysql
import requests

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from app import create_app
from compatibility import checker

CHECKS = {"total": 1, "passed": 1, "warnings": 0, "conflicts": 0}


def part(number, role="peripheral", interfaces="I2C", current=10):
    return {
        "part_number": number, "role": role, "supply_min_v": 3, "supply_max_v": 3.6,
        "logic_output_high_v": 3.3, "logic_input_high_min_v": 2, "logic_input_max_v": 3.6,
        "interfaces": interfaces, "available_gpio": None, "extra_pins": 0, "current_ma": current,
        "clock_mhz": None, "flash_kb": None, "ram_kb": None,
    }


def sample_bom():
    return {
        "supply_min_v": 3.3, "supply_max_v": 3.3, "power_budget_ma": 200,
        "allowed_interfaces": {"I2C", "SPI", "GPIO"}, "min_clock_mhz": 48, "min_flash_kb": 256, "min_ram_kb": 64,
        "items": [
            {"ref": "U1", "part_number": "EX-MCU32", "upstream_ref": None, "interface": None},
            {"ref": "U3", "part_number": "EX-DISP50", "upstream_ref": "U1", "interface": "SPI"},
            {"ref": "C1", "part_number": "USER-LED2", "upstream_ref": "U1", "interface": "GPIO"},
        ],
        "parts": {
            "EX-MCU32": part("EX-MCU32", "controller", {"I2C", "SPI"}, 40),
            "EX-DISP50": part("EX-DISP50", interfaces="SPI", current=80),
            "EX-DISP33": part("EX-DISP33", interfaces="SPI", current=50),
            "USER-LED2": part("USER-LED2", interfaces="GPIO", current=None),
        },
    }


def power_finding(severity="pass", known=117, unknown=0):
    return {"severity": severity, "ruleId": "power-budget", "componentIds": [], "message": "",
            "metrics": {"knownMa": known, "budgetMa": 200, "unknownParts": unknown}}


def component(project, ref):
    return next(c for c in project["components"] if c["id"] == ref)


class CheckerTest(unittest.TestCase):
    def test_project_uses_selected_display(self):
        project = checker.build_project(sample_bom(), "EX-DISP33", None)
        self.assertEqual(component(project, "U3")["specs"]["currentMa"]["value"], 50)

    def test_unknown_led_current_is_left_missing(self):
        project = checker.build_project(sample_bom(), "EX-DISP50", None)
        self.assertNotIn("currentMa", component(project, "C1")["specs"])

    def test_entered_led_current_is_applied(self):
        project = checker.build_project(sample_bom(), "EX-DISP50", 15)
        self.assertEqual(component(project, "C1")["specs"]["currentMa"], {"value": 15.0, "status": "verified"})

    def test_summary_of_failing_run(self):
        conflict = {"severity": "conflict", "ruleId": "logic-level", "componentIds": ["U3", "U1"],
                    "message": "", "metrics": {}}
        result = checker.summarize({"status": "conflict", "checks": CHECKS,
                                    "findings": [conflict, power_finding("warning", 132, 1)]})
        self.assertTrue(result["conflict"])
        self.assertFalse(result["known"])
        self.assertFalse(result["ready"])
        self.assertEqual(result["total"], 132)

    def test_summary_passes_check_counts_through(self):
        result = checker.summarize({"status": "pass", "checks": CHECKS, "findings": [power_finding()]})
        self.assertTrue(result["ready"])
        self.assertEqual(result["checks"], CHECKS)

    def test_over_budget_is_not_reported_as_a_logic_conflict(self):
        result = checker.summarize({"status": "conflict", "checks": CHECKS,
                                    "findings": [power_finding("conflict", 250)]})
        self.assertTrue(result["overBudget"])
        self.assertFalse(result["conflict"])


class CheckEndpointTest(unittest.TestCase):
    def setUp(self):
        self.client = create_app().test_client()
        self.body = {"revision": 5, "display": "EX-DISP50", "ledCurrent": None}

    def test_rejects_invalid_input(self):
        for change in ({"ledCurrent": -1}, {"ledCurrent": "15"}, {"display": ["EX-DISP50"]}, {"revision": 0}):
            with self.subTest(change):
                response = self.client.post("/api/check", json={**self.body, **change})
                self.assertEqual(response.status_code, 400)

    def test_rejects_a_part_that_is_not_a_display(self):
        with patch("compatibility.store.load_bom", return_value=sample_bom()):
            response = self.client.post("/api/check", json={**self.body, "display": "EX-MCU32"})
        self.assertEqual(response.status_code, 400)

    def test_reports_engine_outage(self):
        with patch("compatibility.store.load_bom", return_value=sample_bom()), \
                patch("compatibility.checker.evaluate", side_effect=requests.ConnectionError):
            self.assertEqual(self.client.post("/api/check", json=self.body).status_code, 502)

    def test_reports_database_outage(self):
        with patch("compatibility.store.load_bom", side_effect=pymysql.OperationalError):
            self.assertEqual(self.client.post("/api/check", json=self.body).status_code, 503)

    def test_saves_the_run(self):
        report = {"status": "pass", "checks": CHECKS, "findings": [power_finding()]}
        with patch("compatibility.store.load_bom", return_value=sample_bom()), \
                patch("compatibility.checker.evaluate", return_value=report), \
                patch("compatibility.store.save_check_run") as save:
            response = self.client.post("/api/check", json={"revision": 6, "display": "EX-DISP33", "ledCurrent": 15})
        self.assertEqual(response.status_code, 200)
        self.assertTrue(response.get_json()["ready"])
        save.assert_called_once()


if __name__ == "__main__":
    unittest.main()
