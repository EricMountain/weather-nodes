# Weather Station Dashboard Lambda

This Lambda function provides an elegant web dashboard displaying current weather measurements, node statuses, and **integrated historical data graphs** for all connected sensors.

## Features

- **Real-time Measurements**: Current readings from all configured nodes
- **Node Status**: Firmware versions, last update times, and connection status
- **24-Hour Min/Max**: Temperature, humidity, and pressure ranges for the past 24 hours
- **Historical Graphs**: Interactive D3.js-powered charts for visualizing historical data
- **Responsive Design**: Mobile-friendly interface with adaptive layouts
- **Light/Dark Mode**: Automatic theme switching based on system preferences with manual override
- **Progressive Web App**: Installable on mobile devices and desktops

## API Usage

### GET Request

Returns the complete HTML dashboard interface with both current measurements and historical graphs.

**Response**: HTML page with:
- Current sensor readings in elegant cards
- Interactive date/time controls for historical data
- D3.js charts for visualizing trends

### POST Request

Returns JSON data for historical graph generation.

**Form Parameters:**
- `start_date` - ISO format start date (UTC)
- `end_date` - ISO format end date (UTC)
- `metric` - Metric type (temperature, humidity, pressure, battery_voltage, battery_percentage, wifi_dbm, free_heap_bytes)
- `devices` - Comma-separated device IDs

**Response**: JSON object containing time-series data for the selected metric and devices

## Dependencies

- boto3 >= 1.26.0
- D3.js v7 (loaded from CDN)

## Authentication

Requires API key provided either:
- In `X-API-Key` header
- As `api_key` query parameter
- As a cookie (set automatically after first authentication)

The API key must exist in the `api_keys` DynamoDB table and be associated with a valid `device_id`.

## Usage

### Accessing the Dashboard

1. Get your API key from the existing weather nodes system
2. Access the lambda URL with your API key:
   ```text
   https://your-dashboard-url.amazonaws.com/?api_key=YOUR_API_KEY
   ```

### Using Historical Graphs

1. Scroll to the bottom of the dashboard to the "Historical Data" section
2. Select start and end dates/times
3. Choose a metric to visualize (temperature, humidity, etc.)
4. Select which devices to include
5. Click "Generate Graph"

The graph will display:
- Time-series line charts for each sensor
- Interactive tooltips with exact values
- Color-coded legend for multiple devices
- Automatic scaling and formatting

## Architecture

### DynamoDB Tables

- `api_keys`: For authentication
- `device_configs`: Device configuration, nodes, and location settings
- `latest_measurements`: Most recent measurements from each node
- `measurements`: Historical time-series data

### IAM Permissions

- GetItem access to api_keys, device_configs, and latest_measurements tables
- Scan access to latest_measurements table (for graphs device discovery)
- Query access to measurements table (for historical data)

## Deployment

Deploy using Terraform from the `aws/lambdas` directory:

```bash
cd aws/lambdas
terraform init
terraform plan
terraform apply -auto-approve
```

The Lambda function URL will be output after deployment.

## Testing

Run the test suite to verify functionality:

```bash
cd aws/lambdas/dashboard
python3 test-dashboard-lambda.py
```

This tests:
- Authentication (API key validation)
- GET request (HTML dashboard generation)
- POST request (historical data fetching)
- Error handling

## Styling and Theming

The dashboard uses a sophisticated color scheme that adapts to light and dark modes:

**Light Mode:**
- Warm, paper-like background (#f6f4ec)
- High contrast text for readability
- Subtle shadows and borders

**Dark Mode:**
- Deep blue-black background (#0f1115)
- Muted colors to reduce eye strain
- Enhanced shadows for depth

Theme switching:
- Click the location name/emoji in the header to cycle themes
- Cycles between: System → Light → Dark → System
- Preference is saved in localStorage

## Graph Integration Details

The graphs functionality has been integrated from the separate `graphs` lambda into the dashboard:

### What was integrated:
1. **Data fetching functions**: `get_available_devices_for_graphs()`, `get_measurements_data()`, `process_measurements_for_metric()`
2. **POST handler**: Processes graph data requests
3. **HTML/CSS**: Styled to match the dashboard's elegant theme
4. **JavaScript**: D3.js chart rendering with consistent colors and interactions
5. **IAM permissions**: Added Scan permission for device discovery

### Styling consistency:
- Uses the same CSS variables for colors
- Respects light/dark theme settings
- Matches card style and rounded corners
- Consistent typography and spacing

## Security

- API key authentication required for all requests
- Device access limited by API key configuration
- Automatic cookie management for session persistence
- HTTPS recommended for production deployments
- No sensitive data exposed in client-side code

## Troubleshooting

### Common Issues

1. **"API key missing" error**
   - Include API key in URL: `?api_key=YOUR_KEY`
   - Or provide in `X-API-Key` header
   - Verify API key exists in DynamoDB

2. **No measurements displayed**
   - Check device_configs table has nodes configured
   - Verify measurements exist in latest_measurements table
   - Check that device_id in nodes matches actual reporting devices

3. **Graphs not loading**
   - Verify internet connection (D3.js loads from CDN)
   - Check browser console for JavaScript errors
   - Ensure date range includes data points
   - Verify selected devices have measurements

4. **Theme not persisting**
   - Check browser allows localStorage
   - Try clearing site data and reloading

5. **Lambda timeout on graphs**
   - Reduce date range for query
   - Check DynamoDB read capacity
   - Consider pagination for large datasets

## Performance Considerations

- The dashboard queries latest_measurements once per node
- Historical graphs scan latest_measurements for device list
- Date range queries on measurements table use efficient KeyConditionExpression
- Consider adding caching for frequently accessed data
- D3.js renders client-side, reducing Lambda compute time

## Future Enhancements

Potential improvements:
- Add zoom/pan controls for graphs
- Export graph data as CSV
- Multiple metric comparison on same chart
- Real-time updates via WebSocket
- Aggregated views (hourly/daily averages)
- Weather alerts and notifications
