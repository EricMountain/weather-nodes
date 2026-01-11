"""
Authentication utilities shared across lambdas.
"""
from typing import Dict, Any, Optional, Tuple
from http.cookies import SimpleCookie
from urllib.parse import quote
import boto3
from .dynamodb import dynamo_to_python

dynamodb = boto3.client("dynamodb")
API_KEY_COOKIE_NAME = "weather_nodes_api_key"
API_KEY_COOKIE_MAX_AGE = 30 * 24 * 60 * 60  # 30 days


def extract_api_key(event: Dict[str, Any]) -> Optional[str]:
    """Extract API key from headers, query parameters, or cookies."""
    headers = event.get("headers") or {}

    # Check headers first
    api_key = headers.get("x-api-key") or headers.get("X-API-Key")

    # Also check for the api key passed as a query parameter
    if not api_key and event.get("queryStringParameters"):
        qs_params = event.get("queryStringParameters") or {}
        api_key = qs_params.get("api_key")

    # Finally, look for the API key stored as a cookie
    if not api_key:
        cookie_header = headers.get("Cookie") or headers.get("cookie")
        if cookie_header:
            cookie = SimpleCookie()
            try:
                cookie.load(cookie_header)
            except (ValueError, TypeError):
                cookie = None
            if cookie and API_KEY_COOKIE_NAME in cookie:
                api_key = cookie[API_KEY_COOKIE_NAME].value

    return api_key


def build_api_key_cookie(api_key: str) -> str:
    """Build the Set-Cookie header value for the API key."""
    encoded_value = quote(api_key, safe="")
    return (
        f"{API_KEY_COOKIE_NAME}={encoded_value}; "
        f"Path=/; Max-Age={API_KEY_COOKIE_MAX_AGE}; SameSite=Lax; Secure"
    )


def add_cookie_header(headers: Dict[str, str], api_key: Optional[str]) -> Dict[str, str]:
    """Attach the API key cookie to the response headers when available."""
    if not api_key:
        return headers
    updated_headers = dict(headers)
    updated_headers["Set-Cookie"] = build_api_key_cookie(api_key)
    return updated_headers


def authenticate_api_key(api_key: str) -> Tuple[bool, Optional[str], str, Optional[str]]:
    """
    Authenticate API key and return (is_valid, device_id, error_message)
    
    Returns:
        Tuple of (is_valid: bool, device_id: Optional[str], error_message: str)
    """
    if not api_key:
        return False, None, "API key missing", None
    
    try:
        api_key_response = dynamodb.get_item(
            TableName="api_keys", Key={"api_key": {"S": api_key}}
        )
        
        if "Item" not in api_key_response:
            return False, None, "Unauthorized: Invalid API key", None

        api_key_response_item = dynamo_to_python(api_key_response["Item"])
        
        if "device_id" not in api_key_response_item:
            return False, None, "Unauthorized: Device ID not found", None

        device_id = api_key_response_item["device_id"]
        return True, device_id, "", api_key_response_item
        
    except Exception as e:
        return False, None, f"Error checking API key: {str(e)}", None