# Deployment Guide for Weather Nodes Graphs Lambda

## Overview

This guide explains how to deploy the new graphs lambda function that generates interactive HTML pages with historical weather data visualizations.

## Prerequisites

- AWS CLI configured with appropriate credentials
- Terraform installed
- Access to the existing weather nodes AWS infrastructure

## Deployment Steps

### 1. Navigate to the lambdas directory

```bash
cd aws/lambdas
```

### 3. Initialize and apply Terraform

```bash
terraform init
terraform plan
terraform apply
```

### 4. Get the lambda URL

After deployment, Terraform will output the lambda URL:

``` text
graphs_lambda_url = "https://xxxxxxxxxx.lambda-url.region.on.aws/"
```

## Testing

### 1. Basic functionality test

```bash
python3 test-graphs-lambda.py
```

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

## Usage

### Accessing the Interface

1. Get your API key from the weather nodes system
2. Visit: `https://your-lambda-url/?api_key=YOUR_API_KEY`

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
