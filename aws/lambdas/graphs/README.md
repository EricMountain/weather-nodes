# Weather Nodes Graphs Lambda

This lambda function generates interactive HTML pages with graphs from historical measurement data.

## Usage

### Accessing the Interface

1. Get your API key from the existing weather nodes system
2. Access the lambda URL with your API key as a query parameter:

   ```text
   https://your-lambda-url.amazonaws.com/?api_key=YOUR_API_KEY
   ```

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

## Deployment

1. Deploy using Terraform:

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

## Future Enhancements

- Export graphs as PNG/SVG
- Aggregation options (hourly, daily averages)
- Statistical overlays (min/max, averages)
- Multiple metric display on same graph
- Custom date range presets
- Real-time data updates
