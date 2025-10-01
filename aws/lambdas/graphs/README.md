# Weather Nodes Graphs Lambda

This lambda function generates interactive HTML pages with graphs from historical weather measurement data.

## Features

- **Interactive Web Interface**: HTML page with controls for selecting date ranges, metrics, and devices
- **Real-time Graphing**: Uses D3.js to create interactive line charts
- **API Key Authentication**: Secure access using existing API key infrastructure
- **Multiple Metrics Support**: Temperature, humidity, pressure, battery voltage, WiFi signal strength
- **Device Selection**: Choose which devices to include in graphs
- **Responsive Design**: Works on desktop and mobile devices

## Usage

### Accessing the Interface

1. Get your API key from the existing weather nodes system
2. Access the lambda URL with your API key as a query parameter:
   ```
   https://your-lambda-url.amazonaws.com/?api_key=YOUR_API_KEY
   ```

### Using the Interface

1. **Date Range**: Select start and end dates/times for the data you want to visualize
2. **Metric**: Choose from available metrics:
   - Temperature (°C)
   - Humidity (%)
   - Pressure (hPa)
   - Battery Voltage (V)
   - WiFi Signal (dBm)
3. **Devices**: Select which devices to include in the graph
4. **Generate Graph**: Click the button to fetch data and display the interactive graph

### Graph Features

- **Interactive**: Hover over data points to see detailed information
- **Multi-device**: Display data from multiple devices on the same graph
- **Color-coded**: Each device/sensor combination has its own color
- **Legend**: Shows which color represents which device/sensor
- **Zoomable**: Use D3.js built-in zoom and pan features

## API Endpoints

### GET /
Returns the HTML interface for interactive graph generation.

**Headers:**
- `X-API-Key`: Valid API key

**Response:**
- HTML page with graph interface

### POST /
Returns JSON data for graph generation.

**Headers:**
- `X-API-Key`: Valid API key
- `Content-Type`: application/x-www-form-urlencoded

**Body Parameters:**
- `start_date`: ISO 8601 datetime string (UTC)
- `end_date`: ISO 8601 datetime string (UTC)
- `metric`: Metric name (temperature, humidity, pressure, battery, wifi_dbm)
- `devices[]`: Array of device IDs to include

**Response:**
```json
{
  "success": true,
  "metric": "temperature",
  "data": {
    "device_id_1": [
      {
        "timestamp": "2023-10-01T12:00:00Z",
        "value": 25.5,
        "device_name": "bme680"
      }
    ]
  },
  "start_date": "2023-10-01T00:00:00Z",
  "end_date": "2023-10-01T23:59:59Z"
}
```

## Deployment

1. Build the lambda package:
   ```bash
   ./build-graphs-lambda.sh
   ```

2. Deploy using Terraform:
   ```bash
   terraform plan
   terraform apply
   ```

## Architecture

The lambda integrates with the existing weather nodes infrastructure:

- **DynamoDB Tables**:
  - `api_keys`: For authentication
  - `device_configs`: For device configuration and available nodes
  - `measurements`: For historical measurement data

- **IAM Permissions**:
  - Read access to API keys table
  - Read access to device configs table
  - Query access to measurements table

## Security

- API key authentication required for all requests
- CORS enabled for web browser access
- Device access limited by API key configuration
- No sensitive data exposed in client-side code

## Dependencies

- `boto3`: AWS SDK for Python (included in Lambda runtime)
- `d3.js`: Loaded from CDN for client-side graphing

## Troubleshooting

### Common Issues

1. **"API key missing" error**: Ensure the `X-API-Key` header is included in requests
2. **"No data found"**: Check that the selected date range contains measurements
3. **Graph not displaying**: Check browser console for JavaScript errors
4. **Slow loading**: Large date ranges may take longer to process

### Logs

Enable CloudWatch logging by uncommenting the IAM policy attachment in the Terraform configuration:

```terraform
resource "aws_iam_role_policy_attachment" "graphs_lambda_attach_lambda_basic_execution" {
  role       = aws_iam_role.iam_for_graphs_lambda.name
  policy_arn = "arn:aws:iam::aws:policy/service-role/AWSLambdaBasicExecutionRole"
}
```

## Future Enhancements

- Export graphs as PNG/SVG
- Aggregation options (hourly, daily averages)
- Statistical overlays (min/max, averages)
- Multiple metric display on same graph
- Custom date range presets
- Real-time data updates