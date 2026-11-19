import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from app import create_app


class BackendApiTest(unittest.TestCase):
    def setUp(self):
        self.client = create_app().test_client()

    def test_pricing_returns_mock_and_unavailable_results(self):
        response = self.client.post(
            "/api/pricing",
            json={"partIds": ["EX-MCU32", "USER-LED2"]},
            headers={"Origin": "http://localhost:5173"},
        )
        self.assertEqual(response.status_code, 200)
        self.assertEqual(
            response.headers.get("Access-Control-Allow-Origin"),
            "http://localhost:5173",
        )
        results = response.get_json()["items"]
        self.assertEqual(results[0]["productNumber"], "STM32F401CCU6")
        self.assertEqual(results[0]["status"], "mock")
        self.assertEqual(results[1]["status"], "unavailable")
        self.assertIsNone(results[1]["price"])

    def test_pricing_rejects_invalid_payload(self):
        response = self.client.post("/api/pricing", json={"partIds": "EX-MCU32"})
        self.assertEqual(response.status_code, 400)

if __name__ == "__main__":
    unittest.main()
