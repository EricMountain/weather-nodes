"""
Dashboard Lambda - Serves an elegant web page with all latest sensor measurements,
node statuses, and version information.
"""
from typing import Dict, Any, List, Optional
import logging
from datetime import datetime, timezone, timedelta
from zoneinfo import ZoneInfo
import base64
import json
from urllib.parse import parse_qs

import boto3
from botocore.exceptions import ClientError

from auth import (
    extract_api_key,
    authenticate_api_key,
    add_cookie_header,
    handle_public_asset_request,
)
from dynamodb import dynamo_to_python
from assets import build_manifest, get_icon_base64, get_favicon_base64

logger = logging.getLogger(__name__)

dynamodb = boto3.client("dynamodb")



def lambda_handler(event: Dict[str, Any], context: Any) -> Dict[str, Any]:
    """Main Lambda handler for dashboard requests."""
    ctx = event.get("requestContext") or {}
    request = ctx.get("http") or {}
    method = request.get("method")
    path = request.get("path", "")

    asset_response = handle_public_asset_request(
        path,
        manifest_builder=lambda: build_manifest(
            name="Weather Station Dashboard",
            short_name="Weather",
            start_path="/",
        ),
        icon_provider=get_icon_base64,
        favicon_provider=get_favicon_base64,
    )
    if asset_response:
        return asset_response

    api_key = extract_api_key(event)
    is_valid, device_id, error_message, _ = authenticate_api_key(api_key)

    if not is_valid:
        if "API key missing" in error_message:
            return {"statusCode": 400, "body": error_message}
        elif "Invalid API key" in error_message:
            return {"statusCode": 401, "body": error_message}
        else:
            return {"statusCode": 500, "body": error_message}

    if method == "POST":
        return handle_post_request(event, device_id, api_key)
    elif method != "GET":
        return {
            "statusCode": 405,
            "headers": add_cookie_header({"Content-Type": "text/plain"}, api_key),
            "body": "Method not allowed",
        }

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
        now_utc = datetime.now(timezone.utc)
        nodes_data = []
        if "nodes" in device_config:
            for node in device_config["nodes"]:
                node_data = get_node_data(node, tz, now_utc)
                if node_data:
                    nodes_data.append(node_data)

        # Get available devices for graphs
        available_devices = get_available_devices_for_graphs(device_id)

        # Generate HTML
        html_content = generate_dashboard_html(nodes_data, device_config, tz, available_devices)

        return {
            "statusCode": 200,
            "headers": add_cookie_header(
                {
                    "Content-Type": "text/html; charset=utf-8",
                },
                api_key,
            ),
            "body": html_content,
        }

    except Exception as e:
        logger.error(f"Error generating dashboard: {str(e)}")
        return {
            "statusCode": 500,
            "headers": add_cookie_header({"Content-Type": "text/plain"}, api_key),
            "body": f"Error generating dashboard: {str(e)}",
        }


def get_node_data(node: Dict[str, Any], tz: ZoneInfo, now_utc: datetime) -> Dict[str, Any]:
    """Fetch latest measurements, status, and 24h min/max for a single node."""
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

        # Fetch 24h min/max aggregates
        min_max = fetch_min_max(node_device_id, now_utc)
        if min_max:
            node_data["measurements_min_max"] = min_max

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

def fetch_min_max(node_device_id: str, now_utc: datetime, hours: int = 24) -> Dict[str, Any]:
    """Fetch 24h min/max for key measurements (temperature, humidity, pressure)."""
    window_start = (now_utc - timedelta(hours=hours)).isoformat(timespec="seconds")
    try:
        measurements_today_response = dynamodb.query(
            TableName="measurements",
            KeyConditionExpression="device_id = :device_id AND timestamp_utc >= :start_timestamp_utc",
            ExpressionAttributeValues={
                ":device_id": {"S": node_device_id},
                ":start_timestamp_utc": {"S": window_start},
            },
        )
    except ClientError as err:
        logger.error(
            "Couldn't query measurements: %s: %s",
            err.response["Error"]["Code"],
            err.response["Error"]["Message"],
        )
        return None

    items = measurements_today_response.get("Items") or []
    if not items:
        return None

    measurements_today = [dynamo_to_python(item) for item in items]
    min_max: Dict[str, Dict[str, Dict[str, float]]] = {}

    for measurement in measurements_today:
        if "measurements_v2" not in measurement:
            continue
        for device, device_measurements in measurement["measurements_v2"].items():
            if device not in min_max:
                min_max[device] = {}
            for measurement_name, measurement_value in device_measurements.items():
                if measurement_name not in ["temperature", "humidity", "pressure"]:
                    continue
                try:
                    value_as_float = float(measurement_value)
                except (ValueError, TypeError):
                    continue

                if measurement_name not in min_max[device]:
                    min_max[device][measurement_name] = {
                        "min": value_as_float,
                        "max": value_as_float,
                    }
                else:
                    if value_as_float < min_max[device][measurement_name]["min"]:
                        min_max[device][measurement_name]["min"] = value_as_float
                    if value_as_float > min_max[device][measurement_name]["max"]:
                        min_max[device][measurement_name]["max"] = value_as_float

    if not min_max:
        return None

    result: Dict[str, Dict[str, Dict[str, str]]] = {}
    for device, device_measurements in min_max.items():
        if device_measurements:
            result[device] = {}
        for measurement_name, measurement_value in device_measurements.items():
            result[device][measurement_name] = {
                "min": str(measurement_value["min"]),
                "max": str(measurement_value["max"]),
            }

    return result

def generate_dashboard_html(
    nodes_data: List[Dict[str, Any]], device_config: Dict[str, Any], tz: ZoneInfo, available_devices: List[Dict[str, str]] = None
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
    
    # Generate device checkboxes for graphs
    device_checkboxes = ""
    if available_devices:
        for i, device in enumerate(available_devices):
            checked = "checked" if i == 0 else ""
            device_checkboxes += f'''
                <div class="device-checkbox">
                    <input type="checkbox" name="device" value="{device['device_id']}" id="graph-{device['device_id']}" {checked}>
                    <label for="graph-{device['device_id']}">{device['display_name']}</label>
                </div>'''

    html = f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta name="color-scheme" content="light dark">
    <meta name="theme-color" media="(prefers-color-scheme: light)" content="#f6f4ec">
    <meta name="theme-color" media="(prefers-color-scheme: dark)" content="#0f1115">
    <meta name="theme-color" id="dynamic-theme-color" content="#f6f4ec">
    <meta name="apple-mobile-web-app-capable" content="yes">
    <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
    <link rel="icon" href="/favicon.ico" type="image/x-icon">
    <link rel="apple-touch-icon" href="/icon-192.png">
    <link rel="manifest" href="/manifest.webmanifest">
    <title>Weather Station Dashboard</title>
    <link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Material+Symbols+Rounded:opsz,wght,FILL,GRAD@24,400,1,0" />
    <script src="https://d3js.org/d3.v7.min.js"></script>
    <style>
        * {{
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }}

        :root {{
            color-scheme: light dark;
            /* Light palette */
            --light-bg: #f6f4ec;
            --light-text: #0f0f0f;
            --light-muted: #4c4c4c;
            --light-card: #ffffff;
            --light-card-border: #dcd6c6;
            --light-accent: #0f0f0f;
            --light-accent-soft: #e5dfd2;
            --light-header-bg: #f6f4ec;
            --light-header-text: #0f0f0f;
            --light-shadow: 0 8px 20px rgba(0, 0, 0, 0.08);
            --light-section-border: #e4dece;
            --light-timestamp: #6c655a;
            --light-chip-bg: #ece6d8;
            --light-status-card: #f7f2e4;
            --light-section-title-bg: #f0ebde;
            --light-section-title-color: #0f0f0f;
            --light-header-gradient: linear-gradient(135deg, #f6f4ec 0%, #e8e0cf 100%);
            --light-node-header-gradient: linear-gradient(135deg, #f6f4ec 0%, #e8e0cf 100%);

            /* Dark palette */
            --dark-bg: #0f1115;
            --dark-text: #f1f1f1;
            --dark-muted: #a4a6ad;
            --dark-card: #151821;
            --dark-card-border: #242a35;
            --dark-accent: #f6f4ec;
            --dark-accent-soft: #1f2430;
            --dark-header-bg: #0f1115;
            --dark-header-text: #f6f4ec;
            --dark-shadow: 0 10px 30px rgba(0, 0, 0, 0.4);
            --dark-section-border: #252b36;
            --dark-timestamp: #b7b9c2;
            --dark-chip-bg: #202634;
            --dark-status-card: #1b212d;
            --dark-section-title-bg: #202634;
            --dark-section-title-color: #f6f4ec;
            --dark-header-gradient: linear-gradient(135deg, #151821 0%, #0f1115 100%);
            --dark-node-header-gradient: linear-gradient(135deg, #242a35 0%, #1b202b 100%);

            /* Active theme defaults to light */
            --bg: var(--light-bg);
            --text: var(--light-text);
            --muted: var(--light-muted);
            --card: var(--light-card);
            --card-border: var(--light-card-border);
            --accent: var(--light-accent);
            --accent-soft: var(--light-accent-soft);
            --header-bg: var(--light-header-bg);
            --header-text: var(--light-header-text);
            --shadow: var(--light-shadow);
            --section-border: var(--light-section-border);
            --timestamp: var(--light-timestamp);
            --chip-bg: var(--light-chip-bg);
            --status-card: var(--light-status-card);
            --section-title-bg: var(--light-section-title-bg);
            --section-title-color: var(--light-section-title-color);
            --header-gradient: var(--light-header-gradient);
            --node-header-gradient: var(--light-node-header-gradient);
        }}

        /* Respect system preference unless user toggles a theme */
        @media (prefers-color-scheme: dark) {{
            :root:not([data-theme]) {{
                --bg: var(--dark-bg);
                --text: var(--dark-text);
                --muted: var(--dark-muted);
                --card: var(--dark-card);
                --card-border: var(--dark-card-border);
                --accent: var(--dark-accent);
                --accent-soft: var(--dark-accent-soft);
                --header-bg: var(--dark-header-bg);
                --header-text: var(--dark-header-text);
                --shadow: var(--dark-shadow);
                --section-border: var(--dark-section-border);
                --timestamp: var(--dark-timestamp);
                --chip-bg: var(--dark-chip-bg);
                --status-card: var(--dark-status-card);
                --section-title-bg: var(--dark-section-title-bg);
                --section-title-color: var(--dark-section-title-color);
                --header-gradient: var(--dark-header-gradient);
                --node-header-gradient: var(--dark-node-header-gradient);
            }}
        }}

        /* Explicit theme overrides from the toggle */
        :root[data-theme="light"] {{
            --bg: var(--light-bg);
            --text: var(--light-text);
            --muted: var(--light-muted);
            --card: var(--light-card);
            --card-border: var(--light-card-border);
            --accent: var(--light-accent);
            --accent-soft: var(--light-accent-soft);
            --header-bg: var(--light-header-bg);
            --header-text: var(--light-header-text);
            --shadow: var(--light-shadow);
            --section-border: var(--light-section-border);
            --timestamp: var(--light-timestamp);
            --chip-bg: var(--light-chip-bg);
            --status-card: var(--light-status-card);
            --section-title-bg: var(--light-section-title-bg);
            --section-title-color: var(--light-section-title-color);
            --header-gradient: var(--light-header-gradient);
            --node-header-gradient: var(--light-node-header-gradient);
        }}

        :root[data-theme="dark"] {{
            --bg: var(--dark-bg);
            --text: var(--dark-text);
            --muted: var(--dark-muted);
            --card: var(--dark-card);
            --card-border: var(--dark-card-border);
            --accent: var(--dark-accent);
            --accent-soft: var(--dark-accent-soft);
            --header-bg: var(--dark-header-bg);
            --header-text: var(--dark-header-text);
            --shadow: var(--dark-shadow);
            --section-border: var(--dark-section-border);
            --timestamp: var(--dark-timestamp);
            --chip-bg: var(--dark-chip-bg);
            --status-card: var(--dark-status-card);
            --section-title-bg: var(--dark-section-title-bg);
            --section-title-color: var(--dark-section-title-color);
            --header-gradient: var(--dark-header-gradient);
            --node-header-gradient: var(--dark-node-header-gradient);
        }}

        body {{
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
            background: var(--bg);
            min-height: 100vh;
            padding: 20px;
            color: var(--text);
            transition: background 0.3s ease, color 0.3s ease;
        }}
        
        .container {{
            max-width: 1200px;
            margin: 0 auto;
        }}
        
        .header {{
            text-align: center;
            color: var(--header-text);
            margin-bottom: 40px;
            background: var(--header-gradient, var(--header-bg));
            padding: 18px 16px;
            border-radius: 14px;
            box-shadow: var(--shadow);
        }}
        
        .header h1 {{
            font-size: 2.3em;
            font-weight: 700;
            letter-spacing: 0.5px;
            cursor: pointer;
        }}
        
        .header p {{
            font-size: 1.05em;
            opacity: 0.9;
            margin-top: 8px;
        }}
        
        .timestamp {{
            text-align: center;
            color: var(--timestamp);
            font-size: 0.9em;
            margin-bottom: 30px;
            cursor: pointer;
        }}

        .timestamp-pill {{
            display: inline-flex;
            align-items: center;
            gap: 6px;
            padding: 6px 12px;
            border-radius: 999px;
            background: var(--chip-bg);
            color: var(--text);
            font-weight: 600;
            border: 1px solid var(--section-border);
        }}

        .timestamp .timestamp-full {{
            display: none;
        }}

        .timestamp.show-full .timestamp-full {{
            display: inline;
            margin-left: 6px;
        }}

        .timestamp.show-full .timestamp-fuzzy {{
            display: none;
        }}

        .theme-badge {{
            display: none;
            margin-left: 10px;
            padding: 4px 10px;
            border-radius: 999px;
            background: var(--chip-bg);
            color: var(--text);
            font-size: 0.8em;
            font-weight: 600;
            border: 1px solid var(--section-border);
        }}

        .timestamp.show-full .theme-badge {{
            display: inline-flex;
            align-items: center;
            gap: 6px;
        }}

        .nodes-grid {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(350px, 1fr));
            gap: 25px;
            margin-bottom: 40px;
        }}
        
        .node-card {{
            background: var(--card);
            border: 1px solid var(--card-border);
            border-radius: 12px;
            box-shadow: var(--shadow);
            overflow: hidden;
            transition: transform 0.3s ease, box-shadow 0.3s ease, border-color 0.3s ease;
        }}
        
        .node-card:hover {{
            transform: translateY(-5px);
            box-shadow: 0 15px 40px rgba(0, 0, 0, 0.18);
            border-color: var(--accent);
        }}
        
        .node-header {{
            background: var(--node-header-gradient, var(--accent));
            color: var(--header-text);
            padding: 20px;
            border-bottom: 3px solid var(--accent-soft);
            cursor: pointer;
            position: relative;
        }}
        
        .node-header h2 {{
            font-size: 1.5em;
            margin-bottom: 5px;
        }}

        .node-header .title-line {{
            display: flex;
            align-items: center;
            gap: 10px;
            flex-wrap: wrap;
        }}

        .node-header .header-right {{
            margin-left: auto;
            display: inline-flex;
            align-items: center;
            gap: 12px;
        }}
        
        .battery-indicator {{
            display: inline-flex;
            align-items: center;
            justify-content: center;
            padding: 6px 8px;
            border-radius: 12px;
            background: rgba(255, 255, 255, 0.12);
            color: var(--header-text);
            border: 1px solid var(--accent-soft);
            box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.14);
        }}

        .battery-indicator .battery-icon {{
            font-family: 'Material Symbols Rounded';
            font-variation-settings:
                'FILL' 1,
                'wght' 400,
                'GRAD' 0,
                'opsz' 24;
            font-size: 28px;
            line-height: 1;
        }}

        .battery-indicator.low {{
            background: rgba(255, 99, 71, 0.18);
            border-color: rgba(255, 99, 71, 0.7);
        }}

        .battery-indicator.mid {{
            background: rgba(255, 193, 7, 0.18);
            border-color: rgba(255, 193, 7, 0.7);
        }}

        .battery-indicator.ok {{
            background: rgba(67, 160, 71, 0.18);
            border-color: rgba(67, 160, 71, 0.7);
        }}
        
        .node-id {{
            font-size: 0.85em;
            opacity: 0.85;
            font-family: 'Courier New', monospace;
            word-break: break-all;
        }}

        .node-meta {{
            display: none;
            margin-top: 12px;
            padding-top: 12px;
            border-top: 1px solid var(--accent-soft);
        }}

        .measurement-age {{
            font-size: 0.9em;
            opacity: 0.9;
            color: var(--timestamp);
            margin-left: 0;
            text-align: right;
        }}

        .node-header.show-id .node-meta {{
            display: block;
        }}

        .node-meta .version {{
            background: var(--chip-bg);
            color: var(--text);
            padding: 10px;
            border-radius: 8px;
            text-align: center;
            font-size: 0.9em;
            margin-top: 10px;
        }}

        .node-meta .last-measurement {{
            background: var(--chip-bg);
            color: var(--text);
            padding: 10px;
            border-radius: 8px;
            text-align: center;
            font-size: 0.9em;
            margin-top: 10px;
        }}

        .node-meta .version-label {{
            font-size: 0.8em;
            opacity: 0.9;
        }}

        .node-meta .version-value {{
            font-size: 1.05em;
            font-weight: 600;
            margin-top: 3px;
            word-break: break-all;
        }}

        .node-meta .last-measurement-label {{
            font-size: 0.8em;
            opacity: 0.9;
        }}

        .node-meta .last-measurement-value {{
            font-size: 1.05em;
            font-weight: 600;
            margin-top: 3px;
            word-break: break-all;
        }}
        
        .node-content {{
            padding: 20px;
        }}
        
        .measurement-section {{
            margin-bottom: 20px;
        }}

        .extra-measurements {{
            display: none;
        }}

        .node-content.show-status .extra-measurements {{
            display: block;
        }}

        .node-content .status-section {{
            display: none;
        }}

        .node-content.show-status .status-section {{
            display: block;
        }}

        .node-content.show-status .status-section .measurement-row {{
            display: flex;
        }}
        
        .section-title {{
            font-size: 0.95em;
            font-weight: 600;
            color: var(--section-title-color);
            text-transform: uppercase;
            letter-spacing: 0.5px;
            margin-bottom: 12px;
            border-bottom: 2px solid var(--section-border);
            background: var(--section-title-bg);
            padding: 10px 12px;
            border-radius: 10px;
        }}
        
        .measurement-row {{
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 8px 0;
            font-size: 0.95em;
        }}

        .measurement-row.primary-metric {{
            flex-direction: column;
            align-items: flex-start;
            padding: 10px 0;
            gap: 2px;
            text-align: center;
        }}
        
        .measurement-label {{
            color: var(--muted);
            font-weight: 500;
        }}
        
        .measurement-value {{
            color: var(--text);
            font-weight: 600;
            font-family: 'Courier New', monospace;
        }}

        .primary-metric .measurement-value {{
            width: 100%;
        }}

        .measurement-value .measurement-minmax {{
            font-size: 0.9em;
            color: var(--muted);
            font-weight: 600;
        }}

        .measurement-value .measurement-current {{
            font-weight: 700;
            color: var(--text);
        }}

        .primary-metric .metric-minmax-line {{
            font-size: 2em;
            color: var(--muted);
            font-weight: 600;
            line-height: 1.2;
        }}

        .primary-metric .metric-current-line {{
            font-size: 4em;
            font-weight: 700;
            color: var(--text);
            font-family: 'Courier New', monospace;
            line-height: 1.1;
        }}
        
        .version {{
            background: var(--accent-soft);
            color: var(--accent);
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
            color: var(--timestamp);
            margin-top: 15px;
            padding-top: 15px;
            border-top: 1px solid var(--section-border);
        }}
        
        .no-data {{
            color: var(--muted);
            font-style: italic;
            padding: 10px 0;
        }}

        /* Graphs Section Styles */
        .graphs-section {{
            margin-top: 60px;
            background: var(--card);
            border: 1px solid var(--card-border);
            border-radius: 12px;
            box-shadow: var(--shadow);
            padding: 30px;
        }}

        .graphs-header {{
            text-align: center;
            margin-bottom: 30px;
        }}

        .graphs-header h2 {{
            font-size: 1.8em;
            font-weight: 700;
            color: var(--text);
        }}

        .graphs-controls {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
            padding: 20px;
            background: var(--accent-soft);
            border-radius: 8px;
        }}

        .control-group {{
            display: flex;
            flex-direction: column;
        }}

        .control-group label {{
            font-weight: 600;
            margin-bottom: 5px;
            color: var(--text);
            font-size: 0.9em;
        }}

        .control-group input,
        .control-group select {{
            padding: 8px;
            border: 1px solid var(--card-border);
            border-radius: 6px;
            font-size: 14px;
            background: var(--card);
            color: var(--text);
            transition: border-color 0.3s ease;
        }}

        .control-group input:focus,
        .control-group select:focus {{
            outline: none;
            border-color: var(--accent);
        }}

        .control-group button {{
            background: var(--accent);
            color: var(--bg);
            border: none;
            padding: 10px 20px;
            border-radius: 6px;
            cursor: pointer;
            font-size: 16px;
            font-weight: 600;
            transition: opacity 0.3s ease, transform 0.1s ease;
        }}

        .control-group button:hover {{
            opacity: 0.9;
            transform: translateY(-1px);
        }}

        .control-group button:active {{
            transform: translateY(0);
        }}

        .control-group button:disabled {{
            opacity: 0.5;
            cursor: not-allowed;
        }}

        .device-selection {{
            display: flex;
            flex-wrap: wrap;
            gap: 10px;
            margin-top: 10px;
        }}

        .device-checkbox {{
            display: flex;
            align-items: center;
            gap: 5px;
        }}

        .device-checkbox input[type="checkbox"] {{
            width: auto;
            cursor: pointer;
        }}

        .device-checkbox label {{
            cursor: pointer;
            font-weight: normal;
            margin: 0;
        }}

        #chart {{
            width: 100%;
            min-height: 500px;
            margin-top: 20px;
            border: 1px solid var(--card-border);
            border-radius: 8px;
            background: var(--bg);
        }}

        .loading {{
            text-align: center;
            padding: 40px;
            color: var(--muted);
        }}

        .error {{
            color: #dc3545;
            background: var(--status-card);
            border: 1px solid #f5c6cb;
            padding: 10px;
            border-radius: 4px;
            margin: 10px 0;
        }}

        .axis {{
            font-size: 12px;
        }}

        .line {{
            fill: none;
            stroke-width: 2px;
        }}

        .dot {{
            stroke-width: 1.5px;
        }}

        .legend {{
            font-size: 12px;
        }}

        .tooltip {{
            position: absolute;
            padding: 8px;
            background: rgba(0, 0, 0, 0.8);
            color: white;
            border-radius: 4px;
            pointer-events: none;
            font-size: 12px;
        }}
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1 class="theme-toggle" title="Click to toggle theme">🌤️ {location_name}</h1>
        </div>

        <div class="nodes-grid">
            {measurements_html}
        </div>

        <div class="timestamp" data-timestamp="{now}">
            <span class="timestamp-fuzzy timestamp-pill"></span>
            <span class="timestamp-full timestamp-pill">Updated {now}</span>
            <span class="theme-badge" aria-label="Theme"></span>
        </div>

        <!-- Historical Graphs Section -->
        <div class="graphs-section">
            <div class="graphs-header">
                <h2>📊 Historical Data</h2>
            </div>
            
            <div class="graphs-controls">
                <div class="control-group">
                    <label for="start-date">Start Date:</label>
                    <input type="datetime-local" id="start-date" required>
                </div>
                
                <div class="control-group">
                    <label for="end-date">End Date:</label>
                    <input type="datetime-local" id="end-date" required>
                </div>
                
                <div class="control-group">
                    <label for="metric">Metric:</label>
                    <select id="metric">
                        <option value="temperature">Temperature (°C)</option>
                        <option value="humidity">Humidity (%)</option>
                        <option value="pressure">Pressure (hPa)</option>
                        <option value="battery_voltage">Battery Voltage (V)</option>
                        <option value="battery_percentage">Battery Percentage (%)</option>
                        <option value="wifi_dbm">WiFi Signal (dBm)</option>
                        <option value="free_heap_bytes">Free Heap (bytes)</option>
                    </select>
                </div>
                
                <div class="control-group">
                    <label>Devices:</label>
                    <div id="device-selection" class="device-selection">
                        {device_checkboxes}
                    </div>
                </div>
                
                <div class="control-group">
                    <button id="generate-graph" onclick="generateGraph()">Generate Graph</button>
                </div>
            </div>
            
            <div id="chart"></div>
        </div>
    </div>

    <script>
        document.addEventListener('DOMContentLoaded', () => {{
            const prefersDark = () => window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches;
            const dynamicThemeMeta = document.getElementById('dynamic-theme-color');
            const themeBadge = document.querySelector('.theme-badge');

            const updateThemeBadge = (mode) => {{
                if (!themeBadge) return;
                let label;
                if (mode === 'light') {{
                    label = 'Light';
                }} else if (mode === 'dark') {{
                    label = 'Dark';
                }} else {{
                    label = `Auto (${{prefersDark() ? 'Dark' : 'Light'}})`;
                }}
                themeBadge.textContent = label;
            }};

            const applyTheme = (mode, persist = false) => {{
                if (mode === 'light') {{
                    document.documentElement.setAttribute('data-theme', 'light');
                    dynamicThemeMeta?.setAttribute('content', '#f6f4ec');
                    if (persist) localStorage.setItem('themePreference', 'light');
                    updateThemeBadge('light');
                }} else if (mode === 'dark') {{
                    document.documentElement.setAttribute('data-theme', 'dark');
                    dynamicThemeMeta?.setAttribute('content', '#0f1115');
                    if (persist) localStorage.setItem('themePreference', 'dark');
                    updateThemeBadge('dark');
                }} else {{
                    document.documentElement.removeAttribute('data-theme');
                    dynamicThemeMeta?.setAttribute('content', prefersDark() ? '#0f1115' : '#f6f4ec');
                    if (persist) localStorage.removeItem('themePreference');
                    updateThemeBadge('auto');
                }}
            }};

            const savedPreference = localStorage.getItem('themePreference');
            if (savedPreference === 'light' || savedPreference === 'dark') {{
                applyTheme(savedPreference, false);
            }} else {{
                applyTheme(null, false);
            }}

            const headerTitle = document.querySelector('.header h1.theme-toggle');
            if (headerTitle) {{
                headerTitle.addEventListener('click', () => {{
                    const currentAttr = document.documentElement.getAttribute('data-theme');
                    const systemMode = prefersDark() ? 'dark' : 'light';
                    const opposite = systemMode === 'dark' ? 'light' : 'dark';
                    const current = currentAttr || 'auto';

                    let nextMode;
                    if (current === 'auto') {{
                        nextMode = opposite; // first click: go to the opposite of system
                    }} else if (current === opposite) {{
                        nextMode = 'auto'; // second click: return to auto
                    }} else {{
                        nextMode = opposite; // fallback: force opposite to ensure manual override available
                    }}

                    if (nextMode === 'auto') {{
                        applyTheme(null, true);
                    }} else {{
                        applyTheme(nextMode, true);
                    }}
                }});
            }}

            document.querySelectorAll('.node-header').forEach((header) => {{
                header.addEventListener('click', () => {{
                    header.classList.toggle('show-id');
                }});
            }});

            document.querySelectorAll('.node-content').forEach((content) => {{
                content.addEventListener('click', () => {{
                    content.classList.toggle('show-status');
                }});
            }});

            const pageTimestamp = document.querySelector('.timestamp');
            if (pageTimestamp) {{
                pageTimestamp.addEventListener('click', () => {{
                    pageTimestamp.classList.toggle('show-full');
                }});
            }}

            const formatAgo = (dateString) => {{
                const parsed = new Date(dateString);
                if (isNaN(parsed.getTime())) return '';
                const diffMs = Date.now() - parsed.getTime();
                const sec = Math.max(0, Math.floor(diffMs / 1000));
                if (sec < 45) return 'a few seconds ago';
                const min = Math.floor(sec / 60);
                if (min < 2) return 'a minute ago';
                if (min < 60) return `${{min}} minutes ago`;
                const hours = Math.floor(min / 60);
                if (hours < 2) return 'an hour ago';
                if (hours < 24) return `${{hours}} hours ago`;
                const days = Math.floor(hours / 24);
                if (days < 2) return 'a day ago';
                return `${{days}} days ago`;
            }};

            const refreshAges = () => {{
                document.querySelectorAll('.measurement-age').forEach((el) => {{
                    const ts = el.getAttribute('data-measured-at');
                    if (!ts) return;
                    const label = formatAgo(ts);
                    el.textContent = label;
                }});

                const tsEl = document.querySelector('.timestamp');
                if (tsEl) {{
                    const raw = tsEl.getAttribute('data-timestamp');
                    const fuzzySpan = tsEl.querySelector('.timestamp-fuzzy');
                    if (raw && fuzzySpan) {{
                        fuzzySpan.textContent = "Updated " + (formatAgo(raw) || raw);
                    }}
                }}
            }};

            refreshAges();
            setInterval(refreshAges, 30000);

            // Initialize graph date range (last 24 hours)
            const now = new Date();
            const yesterday = new Date(now.getTime() - 24 * 60 * 60 * 1000);
            
            const formatDateForInput = (date) => {{
                const year = date.getFullYear();
                const month = String(date.getMonth() + 1).padStart(2, '0');
                const day = String(date.getDate()).padStart(2, '0');
                const hours = String(date.getHours()).padStart(2, '0');
                const minutes = String(date.getMinutes()).padStart(2, '0');
                return `${{year}}-${{month}}-${{day}}T${{hours}}:${{minutes}}`;
            }};
            
            document.getElementById('end-date').value = formatDateForInput(now);
            document.getElementById('start-date').value = formatDateForInput(yesterday);
        }});

        // Chart dimensions
        const margin = {{top: 20, right: 80, bottom: 50, left: 60}};
        const API_KEY_COOKIE_NAME = 'weather_nodes_api_key';
        let width = document.getElementById('chart') ? document.getElementById('chart').offsetWidth - margin.left - margin.right : 800;
        const height = 500 - margin.top - margin.bottom;

        // Color scale for different devices
        const colorScale = d3.scaleOrdinal(d3.schemeCategory10);

        async function generateGraph() {{
            const startDate = document.getElementById('start-date').value;
            const endDate = document.getElementById('end-date').value;
            const metric = document.getElementById('metric').value;
            
            if (!startDate || !endDate) {{
                alert('Please select both start and end dates');
                return;
            }}
            
            const selectedDevices = Array.from(document.querySelectorAll('input[name="device"]:checked'))
                .map(cb => cb.value);
            
            if (selectedDevices.length === 0) {{
                alert('Please select at least one device');
                return;
            }}
            
            // Show loading state
            document.getElementById('chart').innerHTML = '<div class="loading">Loading data...</div>';
            document.getElementById('generate-graph').disabled = true;
            
            try {{
                const response = await fetch(window.location.href, {{
                    method: 'POST',
                    headers: {{
                        'Content-Type': 'application/x-www-form-urlencoded',
                        'X-API-Key': getApiKeyFromUrl()
                    }},
                    body: new URLSearchParams({{
                        start_date: new Date(startDate).toISOString(),
                        end_date: new Date(endDate).toISOString(),
                        metric: metric,
                        devices: selectedDevices
                    }})
                }});
                
                const data = await response.json();
                
                if (data.success) {{
                    drawChart(data);
                }} else {{
                    showError('Error: ' + (data.error || 'Unknown error'));
                }}
                
            }} catch (error) {{
                showError('Error fetching data: ' + error.message);
            }} finally {{
                document.getElementById('generate-graph').disabled = false;
            }}
        }}

        function drawChart(data) {{
            // Clear previous chart
            d3.select('#chart').selectAll('*').remove();
            
            // Prepare data
            const allDataPoints = [];
            const deviceNames = [];
            
            Object.keys(data.data).forEach(deviceId => {{
                const deviceData = data.data[deviceId];
                deviceData.forEach(point => {{
                    allDataPoints.push({{
                        ...point,
                        deviceId: deviceId,
                        date: new Date(point.timestamp)
                    }});
                }});
                deviceNames.push(deviceId);
            }});
            
            if (allDataPoints.length === 0) {{
                document.getElementById('chart').innerHTML = '<div class="loading">No data found for the selected criteria</div>';
                return;
            }}
            
            // Update width based on current chart container
            width = document.getElementById('chart').offsetWidth - margin.left - margin.right;
            
            // Create SVG
            const svg = d3.select('#chart')
                .append('svg')
                .attr('width', width + margin.left + margin.right)
                .attr('height', height + margin.top + margin.bottom);
            
            const g = svg.append('g')
                .attr('transform', `translate(${{margin.left}},${{margin.top}})`);
            
            // Scales
            const xScale = d3.scaleTime()
                .domain(d3.extent(allDataPoints, d => d.date))
                .range([0, width]);
            
            const yScale = d3.scaleLinear()
                .domain(d3.extent(allDataPoints, d => d.value))
                .nice()
                .range([height, 0]);
            
            // Line generator
            const line = d3.line()
                .x(d => xScale(d.date))
                .y(d => yScale(d.value))
                .curve(d3.curveLinear);
            
            // Add axes
            g.append('g')
                .attr('class', 'axis')
                .attr('transform', `translate(0,${{height}})`)
                .call(d3.axisBottom(xScale)
                    .tickFormat(d3.timeFormat('%m/%d %H:%M')));
            
            g.append('g')
                .attr('class', 'axis')
                .call(d3.axisLeft(yScale));
            
            // Add axis labels
            g.append('text')
                .attr('transform', 'rotate(-90)')
                .attr('y', 0 - margin.left)
                .attr('x', 0 - (height / 2))
                .attr('dy', '1em')
                .style('text-anchor', 'middle')
                .text(getMetricLabel(data.metric));
            
            g.append('text')
                .attr('transform', `translate(${{width / 2}}, ${{height + margin.bottom - 10}})`)
                .style('text-anchor', 'middle')
                .text('Time');
            
            // Add tooltip
            const tooltip = d3.select('body').append('div')
                .attr('class', 'tooltip')
                .style('opacity', 0);
            
            // Draw lines for each device
            deviceNames.forEach((deviceId, i) => {{
                const deviceData = allDataPoints.filter(d => d.deviceId === deviceId);
                if (deviceData.length === 0) return;
                
                // Group by device_name within each device
                const groupedData = d3.group(deviceData, d => d.device_name);
                
                groupedData.forEach((points, deviceName) => {{
                    const color = colorScale(`${{deviceId}}-${{deviceName}}`);
                    
                    // Add line
                    g.append('path')
                        .datum(points)
                        .attr('class', 'line')
                        .attr('d', line)
                        .style('stroke', color);
                    
                    // Add dots
                    g.selectAll(`.dot-${{deviceId}}-${{deviceName}}`)
                        .data(points)
                        .enter().append('circle')
                        .attr('class', `dot dot-${{deviceId}}-${{deviceName}}`)
                        .attr('cx', d => xScale(d.date))
                        .attr('cy', d => yScale(d.value))
                        .attr('r', 1)
                        .style('fill', color)
                        .on('mouseover', function(event, d) {{
                            tooltip.transition()
                                .duration(200)
                                .style('opacity', .9);
                            tooltip.html(`Device: ${{deviceId}}<br/>Sensor: ${{d.device_name}}<br/>Value: ${{d.value}}<br/>Time: ${{d.date.toLocaleString()}}`)
                                .style('left', (event.pageX + 10) + 'px')
                                .style('top', (event.pageY - 28) + 'px');
                        }})
                        .on('mouseout', function(d) {{
                            tooltip.transition()
                                .duration(500)
                                .style('opacity', 0);
                        }});
                }});
            }});
            
            // Add legend
            const legend = g.append('g')
                .attr('class', 'legend')
                .attr('transform', `translate(10, 0)`);
            
            let legendY = 0;
            deviceNames.forEach(deviceId => {{
                const deviceData = allDataPoints.filter(d => d.deviceId === deviceId);
                const deviceNames = [...new Set(deviceData.map(d => d.device_name))];
                
                deviceNames.forEach(deviceName => {{
                    const color = colorScale(`${{deviceId}}-${{deviceName}}`);
                    
                    legend.append('rect')
                        .attr('x', 0)
                        .attr('y', legendY)
                        .attr('width', 12)
                        .attr('height', 12)
                        .style('fill', color);
                    
                    legend.append('text')
                        .attr('x', 16)
                        .attr('y', legendY + 6)
                        .attr('dy', '0.35em')
                        .text(`${{deviceId}} - ${{deviceName}}`);
                    
                    legendY += 20;
                }});
            }});
        }}

        function getMetricLabel(metric) {{
            const labels = {{
                'temperature': 'Temperature (°C)',
                'humidity': 'Humidity (%)',
                'pressure': 'Pressure (hPa)',
                'battery': 'Battery Voltage (V)',
                'wifi_dbm': 'WiFi Signal (dBm)',
                'free_heap_bytes': 'Free Heap (bytes)'
            }};
            return labels[metric] || metric;
        }}

        function showError(message) {{
            document.getElementById('chart').innerHTML = `<div class="error">${{message}}</div>`;
        }}

        function getApiKeyFromUrl() {{
            // Extract API key from URL parameters if present, otherwise use the cookie
            const urlParams = new URLSearchParams(window.location.search);
            return urlParams.get('api_key') || getApiKeyFromCookie();
        }}

        function getApiKeyFromCookie() {{
            if (!document.cookie) {{
                return '';
            }}
            const cookies = document.cookie.split(';');
            for (const cookie of cookies) {{
                const trimmed = cookie.trim();
                if (!trimmed) {{
                    continue;
                }}
                const separatorIndex = trimmed.indexOf('=');
                if (separatorIndex === -1) {{
                    continue;
                }}
                const name = trimmed.slice(0, separatorIndex);
                const value = trimmed.slice(separatorIndex + 1);
                if (name === API_KEY_COOKIE_NAME && value) {{
                    try {{
                        return decodeURIComponent(value);
                    }} catch (error) {{
                        return value;
                    }}
                }}
            }}
            return '';
        }}
    </script>
</body>
</html>"""

    return html


def _parse_numeric(value: Any) -> Optional[float]:
    """Best-effort parse of a numeric string/number to float."""
    if value is None:
        return None
    try:
        cleaned = str(value).strip().rstrip("%")
        return float(cleaned)
    except (ValueError, TypeError):
        return None


def extract_battery_data(node: Dict[str, Any]) -> Optional[Dict[str, Any]]:
    """Extract battery percentage/voltage for a node from its measurements."""
    percentage: Optional[float] = None
    voltage: Optional[float] = None

    for device_measurements in node.get("measurements", {}).values():
        for measurement_name, measurement_value in device_measurements.items():
            name_lower = measurement_name.lower()
            value_num = _parse_numeric(measurement_value)

            if "battery_percentage" in name_lower or name_lower == "battery_percent":
                if value_num is not None:
                    percentage = value_num
            elif "battery_voltage" in name_lower or name_lower == "vbat":
                if value_num is not None:
                    voltage = value_num

    if percentage is None and voltage is None:
        return None

    return {
        "percentage": percentage,
        "voltage": voltage,
    }


def _battery_level_class(percentage: Optional[float]) -> str:
    if percentage is None:
        return ""
    if percentage <= 20:
        return "low"
    if percentage <= 55:
        return "mid"
    return "ok"


def _battery_glyph(percentage: Optional[float]) -> str:
    if percentage is None:
        return "battery_unknown"

    clamped = max(0, min(100, percentage))
    if clamped >= 95:
        return "battery_full"
    if clamped >= 75:
        return "battery_5_bar"
    if clamped >= 55:
        return "battery_4_bar"
    if clamped >= 35:
        return "battery_3_bar"
    if clamped >= 15:
        return "battery_2_bar"
    if clamped >= 5:
        return "battery_1_bar"
    return "battery_alert"


def render_battery_indicator(battery_data: Dict[str, Any]) -> str:
    percentage: Optional[float] = battery_data.get("percentage")
    voltage: Optional[float] = battery_data.get("voltage")

    level_class = _battery_level_class(percentage)
    glyph = _battery_glyph(percentage)
    tooltip = ""
    if percentage is not None:
        tooltip = f" title=\"Battery {percentage:.0f}%\""
    elif voltage is not None:
        tooltip = f" title=\"Battery {voltage:.2f}V\""

    return (
        f"<div class=\"battery-indicator {level_class}\"{tooltip}>"
        f"<span class=\"battery-icon material-symbols-rounded\">{glyph}</span>"
        f"</div>"
    )


def render_node_card(node: Dict[str, Any]) -> str:
    """Render an individual node card."""
    display_name = node.get("display_name", node.get("device_id", "Unknown"))
    device_id = node.get("device_id", "Unknown")
    version = node.get("version", "Unknown")

    battery_data = extract_battery_data(node)
    battery_html = render_battery_indicator(battery_data) if battery_data else ""
    measurement_age_html = f'<div class="measurement-age" data-measured-at="{node.get("timestamp_local_str", "")}"></div>'

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
        name = item[1].lower()
        return measurement_priority.get(name, 100)

    # Build measurements HTML
    measurements_html = ""
    if node.get("measurements"):
        measurements_html = '<div class="measurement-section">'

        # Collect and sort measurements
        all_measurements = []
        for device_name, device_measurements in node["measurements"].items():
            for measurement_name, measurement_value in device_measurements.items():
                all_measurements.append((device_name, measurement_name, measurement_value))

        all_measurements.sort(key=get_sort_key)

        primary_metrics = {"temperature", "humidity", "pressure"}
        always_show = primary_metrics
        extra_rows = ""

        measurements_min_max = node.get("measurements_min_max", {})

        for device_name, measurement_name, measurement_value in all_measurements:
            if measurement_name.lower() in primary_metrics:
                row_html = render_primary_measurement(
                    measurement_name,
                    measurement_value,
                    measurements_min_max.get(device_name, {}),
                )
            else:
                row_html = f"""
                    <div class="measurement-row">
                        <span class="measurement-label">{format_measurement_name(measurement_name)}</span>
                        <span class="measurement-value">
                            {render_measurement_with_min_max(measurement_name, measurement_value, measurements_min_max.get(device_name, {}))}
                        </span>
                    </div>
                    """

            if measurement_name.lower() in always_show:
                measurements_html += row_html
            else:
                extra_rows += row_html

        if extra_rows:
            measurements_html += f"""
                <div class="extra-measurements">
                    {extra_rows}
                </div>
            """

        measurements_html += "</div>"

    # Build status HTML
    status_html = ""
    if node.get("status"):
        status_html = '<div class="measurement-section status-section">'
        status_html += '<div class="section-title">📡 Status</div>'

        for status_key, status_value in node["status"].items():
            status_html += f"""
            <div class="measurement-row">
                <span class="measurement-label">{format_measurement_name(status_key)}</span>
                <span class="measurement-value">{format_measurement_value(status_key, status_value)}</span>
            </div>
            """

        status_html += "</div>"

    return f"""
    <div class="node-card">
        <div class="node-header">
            <div class="title-line">
                <h2>{display_name}</h2>
                <div class="header-right">
                    {measurement_age_html}
                    {battery_html}
                </div>
            </div>
            <div class="node-meta">
                <div class="node-id">{device_id}</div>
                <div class="version">
                    <div class="version-label">Firmware Version</div>
                    <div class="version-value">{version}</div>
                </div>
                {f"<div class='last-measurement'><div class='last-measurement-label'>Last Measurement</div><div class='last-measurement-value'>{node['timestamp_local_str']}</div></div>" if 'timestamp_local_str' in node else ''}
            </div>
        </div>
        <div class="node-content">
            {measurements_html}
            {status_html}
        </div>
    </div>
    """


def _lookup_min_max_entry(name: str, min_max: Dict[str, Any]):
    if not min_max:
        return None
    if name in min_max:
        return min_max[name]
    name_lower = name.lower()
    for k, v in min_max.items():
        if k.lower() == name_lower:
            return v
    return None


def render_primary_measurement(
    measurement_name: str, current_value: Any, min_max_for_device: Dict[str, Any]
) -> str:
    """Render primary metrics (temp/humidity/pressure) with stacked min/max over current."""
    min_max_entry = _lookup_min_max_entry(measurement_name, min_max_for_device)
    current_val, unit = format_measurement_parts(measurement_name, current_value)
    unit_suffix = f"{unit}" if unit else ""

    min_max_line = ""
    if min_max_entry and "min" in min_max_entry and "max" in min_max_entry:
        min_val, _ = format_measurement_parts(measurement_name, min_max_entry["min"])
        max_val, _ = format_measurement_parts(measurement_name, min_max_entry["max"])
        unit_suffix_minmax = f"<span class=\"measurement-minmax\">{unit_suffix}</span>" if unit_suffix else ""
        min_max_line = (
            f"<div class=\"measurement-value metric-minmax-line\">"
            f"<span class=\"measurement-minmax\">{min_val}</span>"
            f"{unit_suffix_minmax}"
            f"<span class=\"measurement-minmax\"> - </span>"
            f"<span class=\"measurement-minmax\">{max_val}</span>"
            f"{unit_suffix_minmax}"
            f"</div>"
        )

    current_line = (
        f"<div class=\"measurement-value metric-current-line\">"
        f"{current_val}{unit_suffix}"
        f"</div>"
    )

    return (
        """
        <div class="measurement-row primary-metric">
        """
        + min_max_line
        + current_line
        + """
        </div>
        """
    )


def render_measurement_with_min_max(
    measurement_name: str, current_value: Any, min_max_for_device: Dict[str, Any]
) -> str:
    """Render a measurement value with optional min/current/max trio."""
    min_max_entry = _lookup_min_max_entry(measurement_name, min_max_for_device)
    current_val, unit = format_measurement_parts(measurement_name, current_value)

    if min_max_entry and "min" in min_max_entry and "max" in min_max_entry:
        min_val, min_unit = format_measurement_parts(
            measurement_name, min_max_entry["min"]
        )
        max_val, max_unit = format_measurement_parts(
            measurement_name, min_max_entry["max"]
        )
        unit_suffix = unit or min_unit or max_unit
        unit_suffix = f" {unit_suffix}" if unit_suffix else ""
        return (
            f"<span class=\"measurement-minmax\">{min_val}</span>"
            f"<span class=\"measurement-minmax\">/</span>"
            f"<span class=\"measurement-current\">{current_val}</span>"
            f"<span class=\"measurement-minmax\">/</span>"
            f"<span class=\"measurement-minmax\">{max_val}</span>{unit_suffix}"
        )

    # Fallback to original formatting when no min/max
    return format_measurement_value(measurement_name, current_value)


def format_measurement_parts(name: str, value: Any) -> tuple[str, str]:
    """Return (value_string_without_unit, unit_suffix_with_leading_symbol_if_any)."""
    if value is None:
        return ("N/A", "")

    name_lower = name.lower()
    value_str = str(value).strip()

    unit = ""
    if "temperature" in name_lower:
        unit = "°C"
    elif "humidity" in name_lower:
        unit = "%"
    elif "pressure" in name_lower:
        unit = "hPa"
    elif "battery_voltage" in name_lower:
        unit = "V"
    elif "battery_percentage" in name_lower:
        unit = "%"
    elif name_lower == "wifi_dbm":
        unit = "dBm"
    elif "uptime" in name_lower:
        unit = "s"

    # If unit already present in value_str (case-insensitive), strip it for clean trio
    if unit and value_str.lower().endswith(unit.lower()):
        value_str = value_str[: -len(unit)].strip()
    value_str = apply_rounding(name_lower, value_str)

    return value_str, (unit if unit else "")


def apply_rounding(name_lower: str, value: Any) -> str:
    """Round numeric values based on measurement type."""
    try:
        val = float(value)
    except (ValueError, TypeError):
        return str(value).strip()

    if "temperature" in name_lower:
        return f"{val:.1f}"
    if "humidity" in name_lower or "pressure" in name_lower:
        return f"{int(round(val))}"

    return str(value).strip()


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

    val, unit = format_measurement_parts(name, value)
    return f"{val} {unit}".strip()


def handle_post_request(
    event: Dict[str, Any], device_id: str, api_key: str
) -> Dict[str, Any]:
    """Handle POST request - return graph data as JSON"""
    try:
        body = event.get("body", "")
        if event.get("isBase64Encoded", False):
            body = base64.b64decode(body).decode('utf-8')
        
        params = parse_qs(body)
        
        # Extract parameters
        start_date = params.get("start_date", [""])[0]
        end_date = params.get("end_date", [""])[0]
        metric = params.get("metric", ["temperature"])[0]
        selected_devices = params.get("devices", [""])[0]
        if selected_devices:
            selected_devices = selected_devices.split(',')
        else:
            selected_devices = []
        
        if not start_date or not end_date:
            return {
                "statusCode": 400,
                "headers": add_cookie_header({"Content-Type": "text/plain"}, api_key),
                "body": "start_date and end_date are required",
            }
        
        # Get available devices for this API key
        available_devices = get_available_devices_for_graphs(device_id)
        
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
            "headers": add_cookie_header(
                {
                    "Content-Type": "application/json",
                },
                api_key,
            ),
            "body": json.dumps(measurements_data),
        }
    
    except Exception as e:
        logger.error(f"Error in POST request: {str(e)}")
        return {
            "statusCode": 500,
            "headers": add_cookie_header({"Content-Type": "text/plain"}, api_key),
            "body": f"Error processing request: {str(e)}",
        }


def get_available_devices_for_graphs(device_id: str) -> List[Dict[str, str]]:
    """Get list of all devices from the latest_measurements table"""
    
    def process_scan_items(items: List[Dict]) -> List[Dict[str, str]]:
        """Process a list of DynamoDB scan items to extract device information"""
        devices = []
        for item in items:
            measurement = dynamo_to_python(item)
            device_id_val = measurement.get("device_id")
            if device_id_val:
                # Try to get display name from device_configs
                display_name = device_id_val
                try:
                    config_response = dynamodb.get_item(
                        TableName="device_configs",
                        Key={"device_id": {"S": device_id_val}},
                    )
                    if "Item" in config_response:
                        config = dynamo_to_python(config_response["Item"])
                        if "location" in config and "name" in config["location"]:
                            display_name = config["location"]["name"]
                except Exception:
                    pass
                
                devices.append({
                    "device_id": device_id_val,
                    "display_name": display_name
                })
        return devices
    
    try:
        devices = []
        
        # Scan the latest_measurements table to get all devices
        scan_response = dynamodb.scan(
            TableName="latest_measurements",
        )
        
        if "Items" in scan_response:
            devices.extend(process_scan_items(scan_response["Items"]))
        
        # Handle pagination if there are more results
        while "LastEvaluatedKey" in scan_response:
            scan_response = dynamodb.scan(
                TableName="latest_measurements",
                ExclusiveStartKey=scan_response["LastEvaluatedKey"]
            )
            
            if "Items" in scan_response:
                devices.extend(process_scan_items(scan_response["Items"]))
        
        return devices if devices else [{"device_id": device_id, "display_name": "Main Device"}]
    
    except Exception as e:
        logger.error(f"Error getting available devices: {str(e)}")
        return [{"device_id": device_id, "display_name": "Main Device"}]


def get_measurements_data(device_ids: List[str], start_date: str, end_date: str, metric: str) -> Dict[str, Any]:
    """Get measurements data for the specified devices and date range"""
    try:
        # Parse dates
        start_datetime = datetime.fromisoformat(start_date).replace(tzinfo=timezone.utc)
        end_datetime = datetime.fromisoformat(end_date).replace(tzinfo=timezone.utc)
        
        all_data = {}
        
        for device_id in device_ids:
            try:
                measurements_response = dynamodb.query(
                    TableName="measurements",
                    KeyConditionExpression="device_id = :device_id AND timestamp_utc BETWEEN :start_time AND :end_time",
                    ExpressionAttributeValues={
                        ":device_id": {"S": device_id},
                        ":start_time": {"S": start_datetime.isoformat()},
                        ":end_time": {"S": end_datetime.isoformat()},
                    },
                )
                
                measurements = []
                if "Items" in measurements_response:
                    for item in measurements_response["Items"]:
                        measurement = dynamo_to_python(item)
                        measurements.append(measurement)
                
                # Process measurements to extract the specific metric
                processed_data = process_measurements_for_metric(measurements, metric)
                all_data[device_id] = processed_data
                
            except ClientError as err:
                logger.error(f"Error querying measurements for device {device_id}: {err}")
                all_data[device_id] = []
        
        return {
            "success": True,
            "metric": metric,
            "data": all_data,
            "start_date": start_date,
            "end_date": end_date
        }
    
    except Exception as e:
        logger.error(f"Error getting measurements data: {str(e)}")
        return {
            "success": False,
            "error": str(e)
        }


def process_measurements_for_metric(measurements: List[Dict], metric: str) -> List[Dict]:
    """Process measurements to extract data points for a specific metric"""
    data_points = []
    
    for measurement in measurements:
        timestamp = measurement.get("timestamp_utc", "")
        
        if "measurements_v2" in measurement:
            for device_name, device_measurements in measurement["measurements_v2"].items():
                if metric in device_measurements:
                    try:
                        value = float(device_measurements[metric])
                        data_points.append({
                            "timestamp": timestamp,
                            "value": value,
                            "device_name": device_name
                        })
                    except (ValueError, TypeError):
                        continue
    
    return sorted(data_points, key=lambda x: x["timestamp"])
