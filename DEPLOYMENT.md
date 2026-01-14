# Deployment Guide: Dashboard with Integrated Graphs

## Summary of Changes

This update integrates the graphs lambda functionality directly into the dashboard lambda, providing a unified interface for both current measurements and historical data visualization.

### Key Changes

1. **Dashboard Lambda (`dashboard.py`)**
   - Added POST request handler for fetching historical graph data
   - Integrated data fetching functions: `get_available_devices_for_graphs()`, `get_measurements_data()`, `process_measurements_for_metric()`
   - Added graphs HTML section with date/time controls, metric selector, and device checkboxes
   - Integrated D3.js library for chart rendering
   - Added CSS styling consistent with dashboard theme
   - Added JavaScript for interactive graph generation
   - Total additions: ~700+ lines of code

2. **IAM Permissions (`dashboard-lambda.tf`)**
   - Added `dynamodb:Scan` permission for `latest_measurements` table
   - Required for discovering all available devices for graph selection

3. **Testing (`test-dashboard-lambda.py`)**
   - Comprehensive test suite covering:
     - Authentication
     - GET requests (HTML dashboard)
     - POST requests (graph data)
     - Error handling

4. **Documentation (`README.md`)**
   - Complete usage guide
   - API documentation
   - Troubleshooting tips
   - Architecture overview

## Deployment Steps

### 1. Review Changes

```bash
cd aws/lambdas
git diff HEAD~4 HEAD
```

### 2. Deploy with Terraform

```bash
cd aws/lambdas
terraform init
terraform plan
terraform apply -auto-approve
```

### 3. Get Lambda URL

After deployment, Terraform will output the dashboard URL:

```bash
terraform output dashboard_lambda_url
```

### 4. Test the Dashboard

Access the dashboard with your API key:

```
https://your-dashboard-url.amazonaws.com/?api_key=YOUR_API_KEY
```

### 5. Verify Graphs Functionality

1. Scroll to the bottom of the dashboard
2. You should see the "📊 Historical Data" section
3. Select a date range (default is last 24 hours)
4. Choose a metric (temperature, humidity, etc.)
5. Select devices to include
6. Click "Generate Graph"

## What to Expect

### Before (Separate Lambdas)

- Dashboard lambda: Current measurements only
- Graphs lambda: Separate interface for historical data
- Users had to switch between two different URLs

### After (Integrated)

- Dashboard lambda: Both current measurements AND historical graphs
- Single, unified interface
- Consistent styling and user experience
- Easy navigation between current and historical data

## Visual Changes

The dashboard now includes a new section at the bottom:

```
┌─────────────────────────────────────┐
│  Current Measurements (top)          │
│  - Node cards with latest data       │
│  - Temperature, humidity, etc.       │
│  - 24h min/max values                │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│  📊 Historical Data (bottom, NEW!)   │
│  ┌───────────────────────────────┐  │
│  │ Date/Time Controls            │  │
│  │ - Start date                  │  │
│  │ - End date                    │  │
│  │ - Metric selector             │  │
│  │ - Device checkboxes           │  │
│  │ [Generate Graph] button       │  │
│  └───────────────────────────────┘  │
│  ┌───────────────────────────────┐  │
│  │ Interactive D3.js Chart       │  │
│  │ - Line graphs                 │  │
│  │ - Tooltips on hover           │  │
│  │ - Legend                      │  │
│  └───────────────────────────────┘  │
└─────────────────────────────────────┘
```

## Styling

The graphs section uses the same elegant styling as the rest of the dashboard:

- **Light mode**: Warm, paper-like background
- **Dark mode**: Deep blue-black background
- Consistent with existing color scheme
- Responsive layout
- Smooth transitions

## Performance Notes

- Initial page load may be slightly larger due to D3.js library (~250KB)
- Graph generation is client-side (reduces Lambda compute)
- Historical data queries use efficient DynamoDB KeyConditions
- Scan operation for device discovery is minimal (one per page load)

## Testing Checklist

After deployment, verify:

- [ ] Dashboard loads with current measurements
- [ ] Graphs section appears at bottom
- [ ] Date controls default to last 24 hours
- [ ] All device checkboxes are present
- [ ] "Generate Graph" button works
- [ ] Graph displays correctly with data
- [ ] Tooltips show on hover
- [ ] Legend displays correctly
- [ ] Light/dark theme affects graphs section
- [ ] Mobile view is responsive

## Rollback Plan

If issues occur, you can:

1. Revert the Terraform changes:
   ```bash
   git revert HEAD~4..HEAD
   terraform apply -auto-approve
   ```

2. Or manually restore the old dashboard.py from git history:
   ```bash
   git checkout HEAD~4 -- aws/lambdas/dashboard/dashboard.py
   terraform apply -auto-approve
   ```

## Future Considerations

### Optional: Remove Graphs Lambda

Since graphs functionality is now in the dashboard, you may optionally:

1. Keep the graphs lambda for backward compatibility
2. Or remove it to reduce infrastructure:
   ```bash
   # Comment out or delete graphs-lambda.tf
   terraform apply -auto-approve
   ```

### Performance Optimization

If you notice performance issues:

- Consider adding caching for device list
- Add pagination for large date ranges
- Implement data aggregation for long time periods

## Support

If you encounter issues:

1. Check CloudWatch logs for the dashboard lambda
2. Review browser console for JavaScript errors
3. Verify DynamoDB permissions are correct
4. Run the test suite: `python3 test-dashboard-lambda.py`
5. Check the README.md in the dashboard directory

## Summary

This integration provides a better user experience by combining current and historical data in a single interface, while maintaining the elegant design and responsive layout of the existing dashboard.
