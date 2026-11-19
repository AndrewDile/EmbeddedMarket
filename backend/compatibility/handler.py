import math

import pymysql
import requests
from flask import Blueprint, jsonify, request

from compatibility import checker, store

compatibility_blueprint = Blueprint("compatibility", __name__)

DEMO_BOM_ID = 1


def error(message, status):
    return jsonify({"error": message}), status


def valid_led_current(value):
    if value is None:
        return True
    return (isinstance(value, (int, float)) and not isinstance(value, bool)
            and math.isfinite(value) and value >= 0)


@compatibility_blueprint.get("/api/health")
def health():
    try:
        store.ping()
        checker.ping_engine()
    except (pymysql.MySQLError, requests.RequestException):
        return jsonify({"status": "unavailable"}), 503
    return jsonify({"status": "ok"})


@compatibility_blueprint.post("/api/check")
def check():
    body = request.get_json(silent=True)
    if not isinstance(body, dict):
        return error("Request body must be a JSON object.", 400)
    revision, display, led_current = body.get("revision"), body.get("display"), body.get("ledCurrent")
    if not isinstance(revision, int) or isinstance(revision, bool) or revision < 1:
        return error("revision must be a positive integer.", 400)
    if not isinstance(display, str):
        return error("display must be a part number.", 400)
    if not valid_led_current(led_current):
        return error("ledCurrent must be null or a non-negative number.", 400)

    try:
        bom = store.load_bom(DEMO_BOM_ID)
        if bom is None:
            return error("BOM not found.", 404)
        if display not in bom["parts"] or not display.startswith("EX-DISP"):
            return error("display must be a known display part.", 400)

        report = checker.evaluate(checker.build_project(bom, display, led_current))
        result = checker.summarize(report)
        store.save_check_run(DEMO_BOM_ID, revision, result["ready"], result)
    except pymysql.MySQLError:
        return error("The component database is unavailable.", 503)
    except (requests.RequestException, KeyError, ValueError):
        return error("The rules engine is unavailable.", 502)
    return jsonify(result)
