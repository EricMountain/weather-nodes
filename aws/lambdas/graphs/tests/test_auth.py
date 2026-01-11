import os
import sys

GRAPHS_DIR = os.path.dirname(os.path.dirname(__file__))
sys.path.insert(0, GRAPHS_DIR)

from auth import extract_api_key, API_KEY_COOKIE_NAME


def test_extract_api_key_from_cookie_only():
    event = {
        "headers": {
            "Cookie": f"{API_KEY_COOKIE_NAME}=cookie-api-key"
        }
    }

    assert extract_api_key(event) == "cookie-api-key"


def test_extract_api_key_prefers_header_over_cookie():
    event = {
        "headers": {
            "x-api-key": "header-api-key",
            "Cookie": f"{API_KEY_COOKIE_NAME}=cookie-api-key"
        }
    }

    assert extract_api_key(event) == "header-api-key"


def test_extract_api_key_prefers_query_over_cookie_when_header_missing():
    event = {
        "headers": {
            "Cookie": f"{API_KEY_COOKIE_NAME}=cookie-api-key"
        },
        "queryStringParameters": {
            "api_key": "query-api-key"
        }
    }

    assert extract_api_key(event) == "query-api-key"
