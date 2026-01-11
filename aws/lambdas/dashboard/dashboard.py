"""
Dashboard Lambda - Serves an elegant web page with all latest sensor measurements,
node statuses, and version information.
"""
from typing import Dict, Any, List
import logging
from datetime import datetime, timezone
from zoneinfo import ZoneInfo
import base64

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

    if method != "GET":
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
            padding-bottom: 8px;
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
        
        .measurement-label {{
            color: var(--muted);
            font-weight: 500;
        }}
        
        .measurement-value {{
            color: var(--text);
            font-weight: 600;
            font-family: 'Courier New', monospace;
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
                        fuzzySpan.textContent = "Updated " + formatAgo(raw) || raw;
                    }}
                }}
            }};

            refreshAges();
            setInterval(refreshAges, 30000);
        }});
    </script>
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

        # Collect and sort measurements
        all_measurements = []
        for device_name, device_measurements in node["measurements"].items():
            for measurement_name, measurement_value in device_measurements.items():
                all_measurements.append((measurement_name, measurement_value))

        all_measurements.sort(key=get_sort_key)

        always_show = {"temperature", "humidity", "pressure"}
        extra_rows = ""

        for measurement_name, measurement_value in all_measurements:
            row_html = f"""
                <div class="measurement-row">
                    <span class="measurement-label">{format_measurement_name(measurement_name)}</span>
                    <span class="measurement-value">{format_measurement_value(measurement_name, measurement_value)}</span>
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
                <div class="measurement-age" data-measured-at="{node.get("timestamp_local_str", "")}"></div>
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
