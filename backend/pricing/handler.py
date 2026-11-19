from flask import Blueprint, jsonify, request

from pricing.catalog import price_for

pricing_blueprint = Blueprint("pricing", __name__)


@pricing_blueprint.post("/api/pricing")
def get_pricing():
    body = request.get_json(silent=True)
    part_ids = body.get("partIds") if isinstance(body, dict) else None
    if not isinstance(part_ids, list) or any(
        not isinstance(part_id, str) for part_id in part_ids
    ):
        return jsonify({"error": "partIds must be a list of strings"}), 400

    return jsonify({"items": [price_for(part_id) for part_id in part_ids]})
