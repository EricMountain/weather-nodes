"""
Dashboard Lambda - Serves an elegant web page with all latest sensor measurements,
node statuses, and version information.
"""
from typing import Dict, Any, List
import logging
from datetime import datetime, timezone
from zoneinfo import ZoneInfo

import boto3
from botocore.exceptions import ClientError

from auth import extract_api_key, authenticate_api_key
from dynamodb import dynamo_to_python

logger = logging.getLogger(__name__)

dynamodb = boto3.client("dynamodb")


def lambda_handler(event: Dict[str, Any], context: Any) -> Dict[str, Any]:
    """Main Lambda handler for dashboard requests."""
    ctx = event.get("requestContext") or {}
    request = ctx.get("http") or {}
    method = request.get("method")

    api_key = extract_api_key(event)
    is_valid, device_id, error_message, _ = authenticate_api_key(api_key)

    if not is_valid:
        if "API key missing" in error_message:
            return {"statusCode": 400, "body": error_message}
        elif "Invalid API key" in error_message:
            return {"statusCode": 401, "body": error_message}
        else:
            return {"statusCode": 500, "body": error_message}

    if method != "GET":
        return {"statusCode": 405, "body": "Method not allowed"}

    try:
        # Get device config to find associated nodes
        device_config_response = dynamodb.get_item(
            TableName="device_configs",
            Key={"device_id": {"S": device_id}},
        )
        
        if "Item" in device_config_response:
            device_config = dynamo_to_python(device_config_response["Item"])
        else:
            device_config = {}

        # Get timezone for the device
        if "location" in device_config and "local_timezone" in device_config["location"]:
            tz = ZoneInfo(device_config["location"]["local_timezone"])
        else:
            tz = ZoneInfo("Etc/UTC")

        # Collect data for all nodes
        nodes_data = []
        if "nodes" in device_config:
            for node in device_config["nodes"]:
                node_data = get_node_data(node, tz)
                if node_data:
                    nodes_data.append(node_data)

        # Generate HTML
        html_content = generate_dashboard_html(nodes_data, device_config, tz)

        return {
            "statusCode": 200,
            "headers": {
                "Content-Type": "text/html; charset=utf-8",
            },
            "body": html_content,
        }

    except Exception as e:
        logger.error(f"Error generating dashboard: {str(e)}")
        return {
            "statusCode": 500,
            "headers": {"Content-Type": "text/plain"},
            "body": f"Error generating dashboard: {str(e)}",
        }


def get_node_data(node: Dict[str, Any], tz: ZoneInfo) -> Dict[str, Any]:
    """Fetch latest measurements and status for a single node."""
    try:
        node_device_id = node.get("device_id")
        node_display_name = node.get("display_name", node_device_id)

        latest_measurements_response = dynamodb.get_item(
            TableName="latest_measurements",
            Key={"device_id": {"S": node_device_id}},
        )

        if "Item" not in latest_measurements_response:
            return {
                "device_id": node_device_id,
                "display_name": node_display_name,
                "status": "No data",
            }

        latest_measurement = dynamo_to_python(latest_measurements_response["Item"])

        node_data = {
            "device_id": node_device_id,
            "display_name": node_display_name,
            "measurements": {},
            "status": {},
        }

        # Extract measurements_v2
        if "measurements_v2" in latest_measurement:
            for device_name, device_measurements in latest_measurement[
                "measurements_v2"
            ].items():
                node_data["measurements"][device_name] = device_measurements

        # Extract status
        if "status" in latest_measurement:
            node_data["status"] = latest_measurement["status"]

        # Extract version
        if "version" in latest_measurement:
            node_data["version"] = latest_measurement["version"]

        # Extract and convert timestamp
        if "timestamp_utc" in latest_measurement:
            try:
                timestamp_utc = datetime.fromisoformat(
                    latest_measurement["timestamp_utc"]
                )
                timestamp_local = timestamp_utc.astimezone(tz)
                node_data["timestamp_utc"] = latest_measurement["timestamp_utc"]
                node_data["timestamp_local"] = timestamp_local.isoformat(
                    timespec="seconds"
                )
                node_data["timestamp_local_str"] = timestamp_local.strftime(
                    "%Y-%m-%d %H:%M:%S"
                )
            except (ValueError, TypeError):
                node_data["timestamp_utc"] = latest_measurement["timestamp_utc"]

        return node_data

    except ClientError as err:
        logger.error(
            "Error fetching node data: %s: %s",
            err.response["Error"]["Code"],
            err.response["Error"]["Message"],
        )
        return None
    except Exception as e:
        logger.error(f"Error processing node data: {str(e)}")
        return None


def generate_dashboard_html(
    nodes_data: List[Dict[str, Any]], device_config: Dict[str, Any], tz: ZoneInfo
) -> str:
    """Generate an elegant HTML dashboard page."""
    location_name = "Weather Station"
    if "location" in device_config and "name" in device_config["location"]:
        location_name = device_config["location"]["name"]

    now = datetime.now(tz).strftime("%Y-%m-%d %H:%M:%S")

    # Build the measurements HTML
    measurements_html = ""
    for node in nodes_data:
        measurements_html += render_node_card(node)

    html = f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Weather Station Dashboard</title>
    <style>
        * {{
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }}
        
        body {{
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
            color: #333;
        }}
        
        .container {{
            max-width: 1200px;
            margin: 0 auto;
        }}
        
        .header {{
            text-align: center;
            color: white;
            margin-bottom: 40px;
            text-shadow: 0 2px 4px rgba(0, 0, 0, 0.2);
        }}
        
        .header h1 {{
            font-size: 2.5em;
            margin-bottom: 10px;
            font-weight: 700;
        }}
        
        .header p {{
            font-size: 1.1em;
            opacity: 0.95;
        }}
        
        .timestamp {{
            text-align: center;
            color: rgba(255, 255, 255, 0.8);
            font-size: 0.9em;
            margin-bottom: 30px;
        }}
        
        .nodes-grid {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(350px, 1fr));
            gap: 25px;
            margin-bottom: 40px;
        }}
        
        .node-card {{
            background: white;
            border-radius: 12px;
            box-shadow: 0 10px 30px rgba(0, 0, 0, 0.2);
            overflow: hidden;
            transition: transform 0.3s ease, box-shadow 0.3s ease;
        }}
        
        .node-card:hover {{
            transform: translateY(-5px);
            box-shadow: 0 15px 40px rgba(0, 0, 0, 0.3);
        }}
        
        .node-header {{
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 20px;
            border-bottom: 3px solid rgba(0, 0, 0, 0.1);
        }}
        
        .node-header h2 {{
            font-size: 1.5em;
            margin-bottom: 5px;
        }}
        
        .node-id {{
            font-size: 0.85em;
            opacity: 0.9;
            font-family: 'Courier New', monospace;
        }}
        
        .node-content {{
            padding: 20px;
        }}
        
        .measurement-section {{
            margin-bottom: 20px;
        }}
        
        .section-title {{
            font-size: 0.95em;
            font-weight: 600;
            color: #667eea;
            text-transform: uppercase;
            letter-spacing: 0.5px;
            margin-bottom: 12px;
            border-bottom: 2px solid #eee;
            padding-bottom: 8px;
        }}
        
        .measurement-row {{
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 8px 0;
            border-bottom: 1px solid #f0f0f0;
            font-size: 0.95em;
        }}
        
        .measurement-row:last-child {{
            border-bottom: none;
        }}
        
        .measurement-label {{
            color: #666;
            font-weight: 500;
        }}
        
        .measurement-value {{
            color: #333;
            font-weight: 600;
            font-family: 'Courier New', monospace;
        }}
        
        .version {{
            background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
            color: white;
            padding: 12px;
            border-radius: 6px;
            text-align: center;
            font-size: 0.9em;
            margin-top: 15px;
        }}
        
        .version-label {{
            font-size: 0.8em;
            opacity: 0.9;
        }}
        
        .version-value {{
            font-size: 1.1em;
            font-weight: 600;
            margin-top: 3px;
        }}
        
        .timestamp-info {{
            font-size: 0.85em;
            color: #999;
            margin-top: 15px;
            padding-top: 15px;
            border-top: 1px solid #eee;
        }}
        
        .footer {{
            text-align: center;
            color: rgba(255, 255, 255, 0.8);
            font-size: 0.9em;
            padding: 20px;
        }}
        
        .no-data {{
            color: #999;
            font-style: italic;
            padding: 10px 0;
        }}
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🌤️ Weather Station</h1>
            <p>{location_name}</p>
        </div>
        
        <div class="timestamp">
            Last updated: {now}
        </div>
        
        <div class="nodes-grid">
            {measurements_html}
        </div>
        
        <div class="footer">
            <p>Weather Station Dashboard • Real-time monitoring</p>
        </div>
    </div>
</body>
</html>"""

    return html


def render_node_card(node: Dict[str, Any]) -> str:
    """Render an individual node card."""
    display_name = node.get("display_name", node.get("device_id", "Unknown"))
    device_id = node.get("device_id", "Unknown")
    version = node.get("version", "Unknown")

    # Define measurement order priority
    measurement_priority = {
        "temperature": 1,
        "humidity": 2,
        "pressure": 3,
        "battery": 4,
        "rssi": 5,
        "wifi": 5,
        "wifi_dbm": 5,
    }

    def get_sort_key(item):
        """Get sort priority for a measurement."""
        name = item[0].lower()
        return measurement_priority.get(name, 100)

    # Build measurements HTML
    measurements_html = ""
    if node.get("measurements"):
        measurements_html = '<div class="measurement-section">'
        measurements_html += '<div class="section-title">📊 Measurements</div>'

        # Collect all measurements first
        all_measurements = []
        for device_name, device_measurements in node["measurements"].items():
            for measurement_name, measurement_value in device_measurements.items():
                all_measurements.append((measurement_name, measurement_value))

        # Sort measurements by priority
        all_measurements.sort(key=get_sort_key)

        # Render sorted measurements
        for measurement_name, measurement_value in all_measurements:
            measurements_html += f"""
                <div class="measurement-row">
                    <span class="measurement-label">{format_measurement_name(measurement_name)}</span>
                    <span class="measurement-value">{format_measurement_value(measurement_name, measurement_value)}</span>
                </div>
                """

        measurements_html += "</div>"

    # Build status HTML
    status_html = ""
    if node.get("status"):
        status_html = '<div class="measurement-section">'
        status_html += '<div class="section-title">📡 Status</div>'

        for status_key, status_value in node["status"].items():
            status_html += f"""
            <div class="measurement-row">
                <span class="measurement-label">{format_measurement_name(status_key)}</span>
                <span class="measurement-value">{format_measurement_value(status_key, status_value)}</span>
            </div>
            """

        status_html += "</div>"

    # Build version HTML
    version_html = f"""
    <div class="version">
        <div class="version-label">Firmware Version</div>
        <div class="version-value">{version}</div>
    </div>
    """

    # Build timestamp HTML
    timestamp_html = ""
    if "timestamp_local_str" in node:
        timestamp_html = f"""
        <div class="timestamp-info">
            Last measurement: {node["timestamp_local_str"]}
        </div>
        """

    return f"""
    <div class="node-card">
        <div class="node-header">
            <h2>{display_name}</h2>
            <div class="node-id">{device_id}</div>
        </div>
        <div class="node-content">
            {measurements_html}
            {status_html}
            {version_html}
            {timestamp_html}
        </div>
    </div>
    """


def format_measurement_name(name: str) -> str:
    """Format measurement name to be human-readable."""
    # Convert snake_case to Title Case with emoji
    emoji_map = {
        "temperature": "🌡️",
        "humidity": "💧",
        "pressure": "🔽",
        "battery": "🔋",
        "battery_voltage": "🔌",
        "battery_percentage": "🔋",
        "rssi": "📶",
        "wifi": "📶",
        "wifi_dbm": "📶",
        "uptime": "⏱️",
        "last_update": "🕐",
        "free_heap_bytes": "💾",
        "sht31d": "🌡️",
        "bme680": "🌡️",
    }

    emoji = emoji_map.get(name.lower(), "")
    formatted = name.replace("_", " ").title()
    if emoji:
        return f"{emoji} {formatted}"
    return formatted


def format_measurement_value(name: str, value: Any) -> str:
    """Format measurement value with appropriate units."""
    if value is None:
        return "N/A"

    name_lower = name.lower()
    value_str = str(value)

    # Add units based on measurement type
    if "temperature" in name_lower and "°" not in value_str:
        return f"{value}°C"
    elif "humidity" in name_lower and "%" not in value_str:
        return f"{value}%"
    elif "pressure" in name_lower and "h" not in value_str:
        return f"{value} hPa"
    elif "battery_voltage" in name_lower and "V" not in value_str:
        return f"{value} V"
    elif "battery_percentage" in name_lower and "%" not in value_str:
        return f"{value}%"
    elif "rssi" in name_lower:
        return f"{value} dBm"
    elif "uptime" in name_lower:
        return f"{value}s"

    return value_str
