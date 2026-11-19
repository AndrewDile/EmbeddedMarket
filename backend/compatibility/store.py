"""MySQL access for the compatibility check."""
import json
import os

import pymysql


def connect():
    return pymysql.connect(
        host=os.environ.get("DB_HOST", "db"),
        port=int(os.environ.get("DB_PORT", "3306")),
        user=os.environ.get("DB_USER", "embedded"),
        password=os.environ.get("DB_PASSWORD", "embedded"),
        database=os.environ.get("DB_NAME", "embedded_market"),
        charset="utf8mb4",
        cursorclass=pymysql.cursors.DictCursor,
    )


def ping():
    with connect() as connection:
        connection.ping()


def load_bom(bom_id):
    """Returns the BOM with its items and the part catalog, or None if it does not exist."""
    with connect() as connection, connection.cursor() as cursor:
        cursor.execute("SELECT * FROM boms WHERE id = %s", (bom_id,))
        bom = cursor.fetchone()
        if bom is None:
            return None
        cursor.execute("SELECT * FROM bom_items WHERE bom_id = %s ORDER BY ref", (bom_id,))
        bom["items"] = cursor.fetchall()
        cursor.execute("SELECT * FROM parts")
        bom["parts"] = {part["part_number"]: part for part in cursor.fetchall()}
        return bom


def save_check_run(bom_id, revision, ready, report):
    with connect() as connection, connection.cursor() as cursor:
        cursor.execute(
            "INSERT INTO check_runs (bom_id, revision, ready, report) VALUES (%s, %s, %s, %s)",
            (bom_id, revision, ready, json.dumps(report)),
        )
        connection.commit()
