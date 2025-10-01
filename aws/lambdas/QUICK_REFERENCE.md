# Weather Nodes Graphs Lambda - Quick Reference

## 🚀 Quick Start

1. **Deploy**: `cd aws/lambdas && terraform apply`
2. **Access**: `https://your-lambda-url/?api_key=YOUR_API_KEY`
3. **Use**: Select date range, metric, devices → Click "Generate Graph"

## 📊 Supported Metrics

| Metric | Unit | Description |
|--------|------|-------------|
| `temperature` | °C | Temperature readings |
| `humidity` | % | Humidity percentage |
| `pressure` | hPa | Barometric pressure |
| `battery` | V | Battery voltage |
| `wifi_dbm` | dBm | WiFi signal strength |

## 🔗 API Endpoints

### GET /
Returns interactive HTML interface

**Headers:** `X-API-Key: your-api-key`

### POST /
Returns JSON graph data

**Headers:** 
- `X-API-Key: your-api-key`
- `Content-Type: application/x-www-form-urlencoded`

**Body Parameters:**
- `start_date`: ISO 8601 datetime (UTC)
- `end_date`: ISO 8601 datetime (UTC)
- `metric`: One of the supported metrics
- `devices`: Array of device IDs

## 🛠️ Files Structure

```
aws/lambdas/
├── graphs/
│   ├── graphs.py          # Lambda function
│   ├── requirements.txt   # Dependencies
│   └── README.md         # Documentation
├── graphs-lambda.tf      # Terraform config
├── build-graphs-lambda.sh # Build script
└── test-graphs-lambda.py # Test script
```

## 🔐 Security

- ✅ API key required for all requests
- ✅ Device access limited by API key
- ✅ Input validation and sanitization
- ✅ CORS configured for web access

## 🐛 Troubleshooting

| Issue | Solution |
|-------|----------|
| "API key missing" | Include `?api_key=KEY` in URL or `X-API-Key` header |
| "No data found" | Check date range has measurements |
| Graph not showing | Verify JavaScript console for errors |
| Access denied | Ensure API key has access to selected devices |

## 💡 Features

- 📱 Responsive design (mobile + desktop)
- 🎨 Interactive D3.js charts with hover tooltips
- 🎯 Multi-device and multi-sensor support
- 🔄 Real-time data loading with progress indicators
- 🎨 Color-coded lines with legend
- ⚡ Default 24-hour time range

## 📈 Performance

- **Lambda**: ARM64, 30s timeout, auto-scaling memory
- **Database**: Efficient DynamoDB range queries  
- **Frontend**: CDN-hosted D3.js, client-side rendering
- **Cost**: Pay-per-request, no idle costs