#!/usr/bin/env python3

import random

LIST_OF_NAME = [
    # From Strinova
    "michele",
    "nobunaga",
    "kokona",
    "yvette",
    "flavia",
    "yugiri",
    "leona",
    "chiyo",
    "lawine",
    "meredith",
    "reiichi",
    "kanami",
    "eika",
    "fragrans",
    "mara",
    "celestia",
    "audrey",
    "meddelena",
    "fuchsia",
    "galatea",
    "cielle",
    "lilith",
    # From
]

print(f"@{random.choice(LIST_OF_NAME)}{str(random.randint(0, 2 ** 18))}")
