from typing import Dict, Any, List
import json
import logging
from datetime import datetime, timedelta, timezone
from zoneinfo import ZoneInfo
from urllib.parse import parse_qs

import boto3
from boto3.dynamodb.types import TypeDeserializer, TypeSerializer
from botocore.exceptions import ClientError

logger = logging.getLogger(__name__)

dynamodb = boto3.client("dynamodb")
deserializer = TypeDeserializer()
serializer = TypeSerializer()


def lambda_handler(event: Dict[str, Any], context: Any) -> Dict[str, Any]:
    """
    Lambda handler for generating HTML pages with interactive graphs from historical measurements.
    Supports querying by date range, metric type, and device selection.
    """
    ctx = event.get("requestContext") or {}
    request = ctx.get("http") or {}
    method = request.get("method")
    
    # API key authentication
    headers = event.get("headers") or {}
    # also check for the api key passed as a query parameter
    if not headers.get("x-api-key") and event.get("queryStringParameters"):
        qs_params = event.get("queryStringParameters") or {}
        if qs_params.get("api_key"):
            headers["x-api-key"] = qs_params.get("api_key")
    api_key = headers.get("x-api-key") or headers.get("X-API-Key")
    if not api_key:
        return {"statusCode": 400, "body": "API key missing"}
    
    try:
        api_key_response = dynamodb.get_item(
            TableName="api_keys", Key={"api_key": {"S": api_key}}
        )
        if "Item" not in api_key_response:
            return {"statusCode": 401, "body": "Unauthorized: Invalid API key"}
        api_key_response_item = dynamo_to_python(api_key_response["Item"])
        if "device_id" not in api_key_response_item:
            return {"statusCode": 401, "body": "Unauthorized: Device ID not found"}
        device_id = api_key_response_item["device_id"]
    except Exception as e:
        return {"statusCode": 500, "body": f"Error checking API key: {str(e)}"}

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
        selected_devices = params.get("devices", [])
        
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


def get_available_devices(device_id: str) -> List[Dict[str, str]]:
    """Get list of devices available for the given device_id"""
    try:
        # Get device config
        device_config_response = dynamodb.get_item(
            TableName="device_configs",
            Key={"device_id": {"S": device_id}},
        )
        
        devices = []
        if "Item" in device_config_response:
            device_config = dynamo_to_python(device_config_response["Item"])
            if "nodes" in device_config:
                for node in device_config["nodes"]:
                    devices.append({
                        "device_id": node["device_id"],
                        "display_name": node["display_name"]
                    })
        
        # If no devices found in config, at least include the main device
        if not devices:
            devices.append({
                "device_id": device_id,
                "display_name": "Main Device"
            })
        
        return devices
    
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


def generate_html_interface(available_devices: List[Dict[str, str]] = None) -> str:
    """Generate the HTML interface with interactive controls and D3.js graphs"""
    if not available_devices:
        available_devices = [{"device_id": "displaydev", "display_name": "Display Device"}]
    
    # Generate device checkboxes HTML
    device_checkboxes = ""
    for i, device in enumerate(available_devices):
        checked = "checked" if i == 0 else ""  # Check first device by default
        device_checkboxes += f'''
                <div class="device-checkbox">
                    <input type="checkbox" name="device" value="{device['device_id']}" id="{device['device_id']}" {checked}>
                    <label for="{device['device_id']}">{device['display_name']}</label>
                </div>'''
    
    html_template = '''<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Weather Nodes - Historical Data Graphs</title>
    <script src="https://d3js.org/d3.v7.min.js"></script>
    <style>
        body {{
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            margin: 0;
            padding: 20px;
            background-color: #f5f5f5;
        }}
        
        .container {{
            max-width: 1200px;
            margin: 0 auto;
            background-color: white;
            padding: 30px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }}
        
        h1 {{
            text-align: center;
            color: #333;
            margin-bottom: 30px;
        }}
        
        .controls {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
            padding: 20px;
            background-color: #f8f9fa;
            border-radius: 8px;
        }}
        
        .control-group {{
            display: flex;
            flex-direction: column;
        }}
        
        label {{
            font-weight: bold;
            margin-bottom: 5px;
            color: #555;
        }}
        
        input, select {{
            padding: 8px;
            border: 1px solid #ddd;
            border-radius: 4px;
            font-size: 14px;
        }}
        
        button {{
            background-color: #007bff;
            color: white;
            border: none;
            padding: 10px 20px;
            border-radius: 4px;
            cursor: pointer;
            font-size: 16px;
            transition: background-color 0.3s;
        }}
        
        button:hover {{
            background-color: #0056b3;
        }}
        
        button:disabled {{
            background-color: #6c757d;
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
        
        #chart {{
            width: 100%;
            height: 500px;
            margin-top: 20px;
            border: 1px solid #ddd;
            border-radius: 8px;
        }}
        
        .loading {{
            text-align: center;
            padding: 40px;
            color: #666;
        }}
        
        .error {{
            color: #dc3545;
            background-color: #f8d7da;
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
        <h1>Weather Nodes - Historical Data Graphs</h1>
        
        <div class="controls">
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

    <script>
        // Initialize default date range (last 24 hours)
        const now = new Date();
        const yesterday = new Date(now.getTime() - 24 * 60 * 60 * 1000);
        
        document.getElementById('end-date').value = formatDateForInput(now);
        document.getElementById('start-date').value = formatDateForInput(yesterday);
        
        // Chart dimensions
        const margin = {{top: 20, right: 80, bottom: 50, left: 60}};
        const width = 1000 - margin.left - margin.right;
        const height = 500 - margin.top - margin.bottom;
        
        // Color scale for different devices
        const colorScale = d3.scaleOrdinal(d3.schemeCategory10);
        
        function formatDateForInput(date) {{
            const year = date.getFullYear();
            const month = String(date.getMonth() + 1).padStart(2, '0');
            const day = String(date.getDate()).padStart(2, '0');
            const hours = String(date.getHours()).padStart(2, '0');
            const minutes = String(date.getMinutes()).padStart(2, '0');
            return `${{year}}-${{month}}-${{day}}T${{hours}}:${{minutes}}`;
        }}
        
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
                .attr('transform', `translate(${{width + 10}}, 20)`);
            
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
                'wifi_dbm': 'WiFi Signal (dBm)'
            }};
            return labels[metric] || metric;
        }}
        
        function showError(message) {{
            document.getElementById('chart').innerHTML = `<div class="error">${{message}}</div>`;
        }}
        
        function getApiKeyFromUrl() {{
            // Extract API key from URL parameters if present
            const urlParams = new URLSearchParams(window.location.search);
            return urlParams.get('api_key') || '';
        }}
    </script>
</body>
</html>'''
    
    return html_template.format(device_checkboxes=device_checkboxes)


def dynamo_to_python(dynamo_object: dict) -> dict:
    """Convert DynamoDB item to Python dict"""
    deserializer = TypeDeserializer()
    return {k: deserializer.deserialize(v) for k, v in dynamo_object.items()}


def python_to_dynamo(python_object: dict) -> dict:
    """Convert Python dict to DynamoDB item"""
    serializer = TypeSerializer()
    return {k: serializer.serialize(v) for k, v in python_object.items()}