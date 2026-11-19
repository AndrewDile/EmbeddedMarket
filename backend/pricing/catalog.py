"""Example part mappings and demonstration prices.

All prices in this file are mock values. They are not fetched from a vendor.
"""

PRICING_CATALOG = {
    "EX-MCU32": {
        "productNumber": "STM32F401CCU6",
        "price": 5.25,
        "currency": "USD",
    },
    "EX-I2CMUX8": {
        "productNumber": "TCA9548APWR",
        "price": 0.85,
        "currency": "USD",
    },
    "EX-TEMP33": {
        "productNumber": "TMP117AIDRVR",
        "price": 2.40,
        "currency": "USD",
    },
    # These are fictional example displays, so no real product mapping is claimed.
    "EX-DISP50": {"productNumber": None, "price": 8.00, "currency": "USD"},
    "EX-DISP33": {"productNumber": None, "price": 6.50, "currency": "USD"},
}


def price_for(part_id):
    product = PRICING_CATALOG.get(part_id)
    if product is None:
        return {
            "partId": part_id,
            "productNumber": None,
            "price": None,
            "currency": None,
            "status": "unavailable",
        }

    return {
        "partId": part_id,
        **product,
        "status": "mock",
    }
