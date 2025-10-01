# Deployment Guide for Weather Nodes Graphs Lambda

## Overview

This guide explains how to deploy the new graphs lambda function that generates interactive HTML pages with historical weather data visualizations.

## Prerequisites

- AWS CLI configured with appropriate credentials
- Terraform installed
- Access to the existing weather nodes AWS infrastructure

## Files Created

1. **Lambda Function**: `aws/lambdas/graphs/graphs.py`
2. **Terraform Configuration**: `aws/lambdas/graphs-lambda.tf`
3. **Build Script**: `aws/lambdas/build-graphs-lambda.sh`
4. **Dependencies**: `aws/lambdas/graphs/requirements.txt`
5. **Documentation**: `aws/lambdas/graphs/README.md`
6. **Test Script**: `aws/lambdas/test-graphs-lambda.py`

## Deployment Steps

### 1. Navigate to the lambdas directory
```bash
cd aws/lambdas
```

### 2. Build the lambda package (optional)
```bash
./build-graphs-lambda.sh
```
Note: This step is optional since Terraform will package the lambda automatically.

### 3. Initialize and apply Terraform
```bash
terraform init
terraform plan
terraform apply
```

### 4. Get the lambda URL
After deployment, Terraform will output the lambda URL:
```
graphs_lambda_url = "https://xxxxxxxxxx.lambda-url.region.on.aws/"
```

## Testing the Deployment

### 1. Basic functionality test
```bash
python3 test-graphs-lambda.py
```

### 2. Web interface test
Open the lambda URL in a browser with your API key:
```
https://your-lambda-url/?api_key=YOUR_API_KEY
```

### 3. Full functionality test
1. Select a date range (last 24 hours by default)
2. Choose a metric (temperature, humidity, pressure, battery, wifi_dbm)
3. Select devices to include
4. Click "Generate Graph"

## Configuration

### DynamoDB Permissions
The lambda requires the following DynamoDB permissions:
- Read access to `api_keys` table
- Read access to `device_configs` table  
- Query access to `measurements` table

### CORS Configuration
The lambda is configured with CORS to allow:
- All origins (`*`)
- GET and POST methods
- Headers: `date`, `keep-alive`, `x-api-key`, `content-type`

### Environment Variables
- `LOG_LEVEL`: Set to "INFO" (can be changed to "DEBUG" for more verbose logging)

## Usage

### Accessing the Interface
1. Get your API key from the weather nodes system
2. Visit: `https://your-lambda-url/?api_key=YOUR_API_KEY`

### Features
- **Interactive Controls**: Date range, metric selection, device filtering
- **Real-time Graphs**: D3.js powered interactive line charts
- **Multi-device Support**: Display data from multiple devices simultaneously
- **Responsive Design**: Works on desktop and mobile
- **Hover Details**: Tooltips show precise values and timestamps

### Supported Metrics
- Temperature (°C)
- Humidity (%)
- Pressure (hPa)  
- Battery Voltage (V)
- WiFi Signal Strength (dBm)

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

### Enabling CloudWatch Logs
Uncomment the following in `graphs-lambda.tf`:
```terraform
resource "aws_iam_role_policy_attachment" "graphs_lambda_attach_lambda_basic_execution" {
  role       = aws_iam_role.iam_for_graphs_lambda.name
  policy_arn = "arn:aws:iam::aws:policy/service-role/AWSLambdaBasicExecutionRole"
}
```

### Performance Optimization
- Consider adding DynamoDB Global Secondary Indexes for frequently queried date ranges
- Implement caching for commonly requested data
- Use DynamoDB batch operations for large datasets

## Security Considerations

1. **API Key Protection**: Store API keys securely and rotate regularly
2. **CORS Configuration**: Consider restricting origins to specific domains in production
3. **Rate Limiting**: Consider implementing rate limiting for the lambda
4. **Data Access**: Users can only access data for devices associated with their API key

## Cost Optimization

1. **Lambda Timeout**: Currently set to 30 seconds, adjust based on actual usage
2. **DynamoDB**: Uses PAY_PER_REQUEST billing mode for optimal cost
3. **Data Retention**: Consider implementing TTL on old measurement data

## Monitoring

Set up CloudWatch alarms for:
- Lambda errors and timeouts
- DynamoDB throttling
- High latency responses
- Unusual request patterns