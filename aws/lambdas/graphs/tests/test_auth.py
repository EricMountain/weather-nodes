import os
import sys

GRAPHS_DIR = os.path.dirname(os.path.dirname(__file__))
sys.path.insert(0, GRAPHS_DIR)

from auth import extract_api_key, API_KEY_COOKIE_NAME, add_cookie_header


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


def test_add_cookie_header_sets_cookie_when_api_key_present():
    headers = {"Content-Type": "text/plain"}
    updated = add_cookie_header(headers, "my-key")

    assert headers is not updated  # original dict not mutated
    assert updated["Content-Type"] == "text/plain"
    assert "Set-Cookie" in updated
    assert API_KEY_COOKIE_NAME in updated["Set-Cookie"]


def test_add_cookie_header_noop_without_api_key():
    headers = {"Content-Type": "text/plain"}
    updated = add_cookie_header(headers, None)

    assert updated == headers
