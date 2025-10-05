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

## Troubleshooting

### Common Issues

1. **Lambda deployment fails**
   - Check AWS credentials and permissions
   - Verify Terraform configuration syntax
   - Ensure no conflicting resources exist

2. **"API key missing" error**
   - Ensure API key is included in URL parameters or X-API-Key header
   - Verify API key exists in DynamoDB `api_keys` table

3. **No data displayed**
   - Check that measurements exist for the selected time range
   - Verify device IDs are correct
   - Check DynamoDB table permissions

4. **JavaScript errors**
   - Ensure D3.js CDN is accessible
   - Check browser console for specific errors
   - Verify CORS configuration allows your domain

## Testing

### 1. Basic functionality test

```bash
python3 test-graphs-lambda.py
```
