"""
Authentication utilities for the graphs lambda.
"""
from typing import Dict, Any, Optional, Tuple
import boto3
from .dynamodb import dynamo_to_python

dynamodb = boto3.client("dynamodb")


def extract_api_key(event: Dict[str, Any]) -> Optional[str]:
    """Extract API key from headers or query parameters"""
    headers = event.get("headers") or {}
    
    # Check headers first
    api_key = headers.get("x-api-key") or headers.get("X-API-Key")
    
    # Also check for the api key passed as a query parameter
    if not api_key and event.get("queryStringParameters"):
        qs_params = event.get("queryStringParameters") or {}
        api_key = qs_params.get("api_key")
    
    return api_key


def authenticate_api_key(api_key: str) -> Tuple[bool, Optional[str], str]:
    """
    Authenticate API key and return (is_valid, device_id, error_message)
    
    Returns:
        Tuple of (is_valid: bool, device_id: Optional[str], error_message: str)
    """
    if not api_key:
        return False, None, "API key missing"
    
    try:
        api_key_response = dynamodb.get_item(
            TableName="api_keys", Key={"api_key": {"S": api_key}}
        )
        
        if "Item" not in api_key_response:
            return False, None, "Unauthorized: Invalid API key"
        
        api_key_response_item = dynamo_to_python(api_key_response["Item"])
        
        if "device_id" not in api_key_response_item:
            return False, None, "Unauthorized: Device ID not found"
        
        device_id = api_key_response_item["device_id"]
        return True, device_id, ""
        
    except Exception as e:
        return False, None, f"Error checking API key: {str(e)}"