from flask import Flask
from flask_cors import CORS

from compatibility.handler import compatibility_blueprint
from pricing.handler import pricing_blueprint


def create_app():
    app = Flask(__name__)
    CORS(
        app,
        resources={
            r"/api/*": {
                "origins": [
                    "http://localhost:5173",
                    "http://127.0.0.1:5173",
                ]
            }
        },
    )
    app.register_blueprint(pricing_blueprint)
    app.register_blueprint(compatibility_blueprint)

    return app


if __name__ == "__main__":
    create_app().run(host="0.0.0.0", port=5000)
