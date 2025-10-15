from typing import Dict, Any
import json
import logging
from urllib.parse import parse_qs

from auth import extract_api_key, authenticate_api_key
from datahelper import get_available_devices, get_measurements_data
from htmlhelper import generate_html_interface

logger = logging.getLogger(__name__)


def lambda_handler(event: Dict[str, Any], context: Any) -> Dict[str, Any]:
    """
    Lambda handler for generating HTML pages with interactive graphs from historical measurements.
    Supports querying by date range, metric type, and device selection.
    """

    ctx = event.get("requestContext") or {}
    request = ctx.get("http") or {}
    method = request.get("method")
    
    api_key = extract_api_key(event)
    is_valid, device_id, error_message = authenticate_api_key(api_key)

    if not is_valid:
        if "API key missing" in error_message:
            return {"statusCode": 400, "body": error_message}
        elif "Invalid API key" in error_message:
            return {"statusCode": 401, "body": error_message}
        else:
            return {"statusCode": 500, "body": error_message}

    if method == "GET":
        return handle_get_request(event, device_id)
    elif method == "POST":
        return handle_post_request(event, device_id)
    else:
        return {"statusCode": 405, "body": "Method not allowed"}


def handle_get_request(event: Dict[str, Any], device_id: str) -> Dict[str, Any]:
    """Handle GET request - return the HTML interface"""
    # Get available devices for this API key to populate the interface
    available_devices = get_available_devices(device_id)
    html_content = generate_html_interface(available_devices)
    return {
        "statusCode": 200,
        "headers": {
            "Content-Type": "text/html",
        },
        "body": html_content,
    }


def handle_post_request(event: Dict[str, Any], device_id: str) -> Dict[str, Any]:
    """Handle POST request - return graph data as JSON"""
    try:
        body = event.get("body", "")
        if event.get("isBase64Encoded", False):
            import base64
            body = base64.b64decode(body).decode('utf-8')
        
        params = parse_qs(body)
        
        # Extract parameters
        start_date = params.get("start_date", [""])[0]
        end_date = params.get("end_date", [""])[0]
        metric = params.get("metric", ["temperature"])[0]
        selected_devices = params.get("devices", [])[0].split(',')
        
        if not start_date or not end_date:
            return {"statusCode": 400, "body": "start_date and end_date are required"}
        
        # Get available devices for this API key
        available_devices = get_available_devices(device_id)
        
        # Filter selected devices to only include available ones
        if not selected_devices:
            selected_devices = [d["device_id"] for d in available_devices]
        else:
            valid_device_ids = {d["device_id"] for d in available_devices}
            selected_devices = [d for d in selected_devices if d in valid_device_ids]
        
        # Get measurements data
        measurements_data = get_measurements_data(selected_devices, start_date, end_date, metric)
        
        return {
            "statusCode": 200,
            "headers": {
                "Content-Type": "application/json",
            },
            "body": json.dumps(measurements_data),
        }
    
    except Exception as e:
        logger.error(f"Error in POST request: {str(e)}")
        return {"statusCode": 500, "body": f"Error processing request: {str(e)}"}