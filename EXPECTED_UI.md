# Expected User Interface After Integration

## Dashboard Layout

The integrated dashboard now has two main sections:

### 1. Top Section: Current Measurements (Unchanged)
```
┌──────────────────────────────────────────────────────────┐
│  🌤️ Home Weather Station                                 │
│  (Click to toggle light/dark theme)                      │
└──────────────────────────────────────────────────────────┘

┌────────────────┐  ┌────────────────┐  ┌────────────────┐
│ Indoor Sensor  │  │ Outdoor Sensor │  │ Garage Sensor  │
├────────────────┤  ├────────────────┤  ├────────────────┤
│                │  │                │  │                │
│  Temperature   │  │  Temperature   │  │  Temperature   │
│     22.5°C     │  │     -5.2°C     │  │     15.8°C     │
│   (20-24°C)    │  │   (-8--2°C)    │  │   (12-18°C)    │
│                │  │                │  │                │
│  Humidity      │  │  Humidity      │  │  Humidity      │
│     55%        │  │     85%        │  │     45%        │
│   (50-60%)     │  │   (80-90%)     │  │   (40-50%)     │
│                │  │                │  │                │
│  Pressure      │  │  [Status]      │  │  [Status]      │
│   1013.25 hPa  │  │  WiFi: -45dBm  │  │  Battery: 3.8V │
│ (1010-1015)    │  │  Heap: 123KB   │  │  Uptime: 2h    │
└────────────────┘  └────────────────┘  └────────────────┘

        Updated 2 minutes ago (click for details)
```

### 2. Bottom Section: Historical Graphs (NEW!)
```
┌──────────────────────────────────────────────────────────┐
│  📊 Historical Data                                       │
├──────────────────────────────────────────────────────────┤
│                                                           │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐     │
│  │ Start Date: │  │ End Date:   │  │ Metric:     │     │
│  │ [________]  │  │ [________]  │  │ [▼ Temp   ] │     │
│  └─────────────┘  └─────────────┘  └─────────────┘     │
│                                                           │
│  Devices:                                                 │
│  ☑ Indoor Sensor  ☑ Outdoor Sensor  ☑ Garage Sensor     │
│                                                           │
│  [   Generate Graph   ]                                   │
│                                                           │
│  ┌────────────────────────────────────────────────────┐  │
│  │ 25°C ┤                                             │  │
│  │      │     ╱╲                                      │  │
│  │ 20°C ┤    ╱  ╲    ╱╲                              │  │
│  │      │   ╱    ╲  ╱  ╲                             │  │
│  │ 15°C ┤  ╱      ╲╱    ╲   ╱╲                       │  │
│  │      │ ╱              ╲ ╱  ╲                      │  │
│  │ 10°C ┤╱                ╲╱    ╲                    │  │
│  │      └────────────────────────────────────────    │  │
│  │      00:00   06:00   12:00   18:00   24:00       │  │
│  │                                                    │  │
│  │  Legend:                                           │  │
│  │  ● Indoor - SHT31D   ● Outdoor - BME680           │  │
│  └────────────────────────────────────────────────────┘  │
│                                                           │
│  (Hover over points for exact values)                     │
└──────────────────────────────────────────────────────────┘
```

## Color Schemes

### Light Mode (Default)
- Background: Warm paper-like (#f6f4ec)
- Text: Dark gray (#0f0f0f)
- Cards: White with subtle shadows
- Accents: Dark with soft borders
- Charts: Colorful lines with good contrast

### Dark Mode (Auto or Manual)
- Background: Deep blue-black (#0f1115)
- Text: Off-white (#f1f1f1)
- Cards: Dark blue-gray (#151821)
- Accents: Muted colors for comfort
- Charts: Same colors with dark background

## Interactive Features

1. **Theme Toggle**: Click the header emoji/title
   - Cycles: System → Light → Dark → System
   - Preference saved in browser

2. **Node Cards**: Click to expand/collapse
   - Shows firmware version
   - Shows last measurement time
   - Reveals additional metrics

3. **Graphs Section**: 
   - Date pickers default to last 24 hours
   - Metric dropdown has 7 options
   - Device checkboxes (first one checked by default)
   - Generate button fetches and displays data

4. **Chart Interactions**:
   - Hover over data points for tooltips
   - Shows device, sensor, value, and time
   - Legend identifies each line
   - Auto-scaling for all data points

## Responsive Design

### Desktop (>1200px)
- 3 columns for node cards
- Full-width graphs
- Side-by-side controls

### Tablet (768-1200px)
- 2 columns for node cards
- Full-width graphs
- Stacked controls

### Mobile (<768px)
- 1 column for all elements
- Touch-friendly controls
- Optimized chart size

## What Users Should See

1. **On First Load**:
   - Current measurements at top
   - Graphs section at bottom
   - Date controls default to last 24h
   - First device pre-selected

2. **After Clicking "Generate Graph"**:
   - "Loading data..." message briefly
   - Chart appears with data
   - Multiple lines if multiple sensors
   - Interactive tooltips on hover

3. **On Theme Toggle**:
   - Smooth color transitions
   - All elements update
   - Charts recolor appropriately
   - Preference persists

4. **On Mobile**:
   - All features work
   - Touch-friendly
   - Scrollable content
   - Readable text sizes

## Expected Performance

- Initial load: 1-2 seconds
- Graph generation: <1 second for 24h
- Theme toggle: Instant
- Chart hover: Instant

## Notes

- D3.js loads from CDN (requires internet)
- First device checked by default
- Dates default to last 24 hours
- Temperature is default metric
- All measurements show min/max/current
