# Weather Nodes Graphs Lambda

This Lambda function provides an interactive web interface for visualizing historical weather measurement data with charts and graphs.

## Modules Overview

### `graphs.py`

Main Lambda handler that:

- Handles GET requests (returns HTML interface)
- Handles POST requests (returns JSON data for charts)
- Coordinates authentication and data retrieval

### `utils/auth.py`

Authentication utilities:

- `extract_api_key()` - Extract API key from headers or query params
- `authenticate_api_key()` - Validate API key against DynamoDB

### `utils/data.py`

Data processing utilities:

- `get_available_devices()` - Get devices available for a given API key
- `get_measurements_data()` - Query measurements from DynamoDB
- `process_measurements_for_metric()` - Extract specific metrics from measurements

### `utils/dynamodb.py`

DynamoDB utilities:

- `dynamo_to_python()` - Convert DynamoDB items to Python dictionaries
- `python_to_dynamo()` - Convert Python dictionaries to DynamoDB items

### `utils/html.py`

HTML rendering utilities:

- `generate_html_interface()` - Generate complete HTML page
- `load_static_file()` - Load CSS/JS files
- `load_template()` - Load HTML templates
- `generate_device_checkboxes()` - Generate device selection HTML

### `static/styles.css`

CSS styles for the web interface including:

- Responsive layout
- Chart styling
- Form controls
- Error and loading states

### `static/charts.js`

JavaScript for interactive charts:

- D3.js-based chart rendering
- Data fetching and processing
- User interaction handling
- Date/time utilities

### `templates/index.html`

HTML template for the main interface with placeholders for:

- CSS content injection
- JavaScript content injection
- Device checkbox generation

## API Usage

### GET Request

Returns the HTML interface for viewing graphs.

### POST Request

Returns JSON data for chart generation. Expects form data:

- `start_date` - ISO format start date
- `end_date` - ISO format end date  
- `metric` - Metric type (temperature, humidity, pressure, etc.)
- `devices` - Comma-separated device IDs

## Dependencies

- boto3 - AWS SDK
- botocore - AWS core library

## Authentication

Requires API key provided either:

- In `X-API-Key` header
- As `api_key` query parameter

The API key must exist in the `api_keys` DynamoDB table and be associated with a valid `device_id`.

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
