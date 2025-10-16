# Adding a New Display Type - Implementation Guide

## Quick Reference

To add a new display type (e.g., LCD, OLED), follow these steps:

## Step 1: Create the Header File

Create `src/views/new_display_view.h`:

```cpp
#pragma once

#include "display_view.h"
#include <map>
#include <string>

// Include hardware-specific libraries
#include "model.h"
#include "sensor.h"

/**
 * [Display Type] implementation of DisplayView.
 * Renders to [specific display hardware].
 */
class NewDisplayView : public DisplayView {
 private:
  // Display hardware members
  // Model and state members
  DisplayModel model_;
  DateTime utc_timestamp_;
  DateTime local_timestamp_;
  const JsonDocument* doc_;
  std::map<std::string, Sensor*> sensors_;
  bool needs_refresh_;

  // Helper methods for rendering
  void displayContent();
  void calculateDisplay();
  DateTime parseTimestampValue(const String& timestamp_key);
  DateTime parseTimestamp(const std::string& timestamp,
                         const String& timestamp_key);

 public:
  NewDisplayView();
  ~NewDisplayView() override;

  bool buildModel(const JsonDocument* doc,
                 const std::map<std::string, Sensor*>& sensors) override;
  void render() override;
  bool needsRefresh() const override { return needs_refresh_; }
  void cleanup() override;
};
```

## Step 2: Create the Implementation File

Create `src/views/new_display_view.cpp`:

```cpp
#include "new_display_view.h"

NewDisplayView::NewDisplayView()
    : model_(), doc_(nullptr), needs_refresh_(true) {
  // Initialize hardware
  // Set up pins, communication, etc.
}

NewDisplayView::~NewDisplayView() { cleanup(); }

void NewDisplayView::cleanup() {
  // Turn off display
  // Save power mode
  // Release resources
}

bool NewDisplayView::buildModel(const JsonDocument* doc,
                        const std::map<std::string, Sensor*>& sensors) {
  doc_ = doc;
  sensors_ = sensors;

  // Check if data has changed
  if (doc_ == nullptr || doc_->isNull() || !(*doc_)["nodes"].is<JsonObject>()) {
    needs_refresh_ = true;
    return true;
  }

  // Parse timestamps
  utc_timestamp_ = parseTimestampValue("timestamp_utc");
  local_timestamp_ = parseTimestampValue("timestamp_local");

  // Set date/time in model
  std::string display_date = "(Date unknown)";
  if (local_timestamp_.ok()) {
    display_date = local_timestamp_.niceDate();
  }
  model_.setDateTime(display_date);

  // Build model from JSON
  model_.addNodes((*doc_)["nodes"], utc_timestamp_);

  // Check if refresh needed
  Controller c = Controller(model_);
  needs_refresh_ = c.needRefresh();
  
  return needs_refresh_;
}

void NewDisplayView::render() {
  // Initialize display if needed
  // Clear display
  // Draw content
  // Update display (non-blocking preferred)
  displayContent();
}

void NewDisplayView::displayContent() {
  // Implementation specific to your display
  // Example:
  // - Draw header with date/time
  // - Draw sensor data
  // - Draw status indicators
}

DateTime NewDisplayView::parseTimestampValue(const String& timestamp_key) {
  DateTime dt;
  if ((*doc_)[timestamp_key].is<JsonString>()) {
    std::string timestamp = (*doc_)[timestamp_key].as<String>().c_str();
    dt = parseTimestamp(timestamp, timestamp_key);
  }
  return dt;
}

DateTime NewDisplayView::parseTimestamp(const std::string& timestamp,
                                 const String& timestamp_key) {
  DateTime dt(timestamp);
  if (!dt.ok()) {
    Serial.printf("Failed to parse %s: %s\n", timestamp_key.c_str(),
                  timestamp.c_str());
  }
  return dt;
}
```

## Step 3: Update NodeApp to Use the New View

Edit `src/nodeapp.cpp`:

```cpp
// In the includes section:
#ifdef HAS_DISPLAY
  #ifdef USE_EPD_DISPLAY
    #include "views/epd_view.h"
  #elif defined(USE_LCD_DISPLAY)
    #include "views/lcd_view.h"
  #endif
#endif

// In setup():
#ifdef HAS_DISPLAY
  #ifdef USE_EPD_DISPLAY
    view_ = new EPDView();
  #elif defined(USE_LCD_DISPLAY)
    view_ = new LCDView();
  #endif
#endif
```

## Step 4: Compile with New Display Type

Add to your `platformio.ini`:

```ini
[env:myboard_with_lcd]
platform = espressif32
board = esp32doit-devkit-v1
build_flags = 
    -DUSE_LCD_DISPLAY
    ; ... other flags
```

Or compile from command line:

```bash
platformio run -e myboard_with_lcd
```

## Step 5: Test

1. Verify compilation: `platformio run`
2. Upload to board: `platformio run -t upload`
3. Monitor output: `platformio device monitor`
4. Verify display renders correctly

## Display Interface Reference

### buildModel()
- **Purpose**: Prepare the display model from JSON data
- **Returns**: `true` if display should refresh, `false` if no changes
- **Responsibilities**:
  - Parse JSON data
  - Extract timestamps
  - Calculate derived data (sun/moon positions if needed)
  - Update internal model
  - Compare with previous state to determine if refresh needed

### render()
- **Purpose**: Draw the display
- **Responsibilities**:
  - Clear display if needed
  - Draw all elements
  - Handle any display-specific optimizations
  - Update physical display
  - Note: Called only if `buildModel()` returned `true`

### needsRefresh()
- **Purpose**: Check if display needs refreshing
- **Returns**: `true` if display has changed, `false` otherwise
- **Notes**: 
  - Called before rendering
  - Can be used to batch updates
  - Helps save power on e-paper displays

### cleanup()
- **Purpose**: Safely shut down display
- **Responsibilities**:
  - Power down display
  - Release any resources
  - Save display state (e.paper only)
  - Called before sleep

## Common Implementation Patterns

### Pattern 1: Simple LCD Display
```cpp
void LCDView::render() {
  // Position 0,0 - Date/Time
  lcd.setCursor(0, 0);
  lcd.print(model_.getDateTime());
  
  // Position 0,1 - Temperature
  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  // ... format and print temperature
}
```

### Pattern 2: E-Paper with Refresh Control
```cpp
void EPDView::render() {
  if (!needs_refresh_) return;  // Already drawn
  
  display->setFullWindow();
  display->firstPage();
  do {
    // Draw everything on each pass
    drawHeader();
    drawSensorData();
    drawFooter();
  } while (display->nextPage());
}
```

### Pattern 3: Web Display
```cpp
void WebView::render() {
  std::string html = buildHTML();
  httpServer.send(200, "text/html", html.c_str());
}

std::string WebView::buildHTML() {
  // Generate HTML from model_
  return html;
}
```

## Data Available to Views

The `buildModel()` method receives:
- `doc`: JSON document with all sensor data from API
- `sensors`: Map of local sensors (BME680, SHT31D, Battery, WiFi)

From the model, you can access:
- Date/time information
- Sun/moon positions
- Node data (remote sensors)
- Measurement history (min/max values)
- Battery levels
- Status information

Example:
```cpp
// From model_:
model_.getDateTime()          // Date/time string
model_.getSunRise()           // Sunrise time
model_.getMoonPhaseLetter()   // Moon phase character
model_.getNodeData()          // All node information

// From local sensors:
sensors_["bme680"]->read()    // Temperature, humidity, pressure
sensors_["battery"]->read()   // Battery voltage, percentage
```

## Best Practices

1. **Minimize render() calls**: Only render when data changes
2. **Use model_**: Don't parse JSON directly in render()
3. **Handle errors gracefully**: Check pointers and valid data
4. **Consider power**: Some displays (e-paper) should be powered down
5. **Make it responsive**: Don't block in render() for hardware I/O
6. **Document limitations**: What data this display can show
7. **Test independently**: Mock JSON data for testing

## Integration Checklist

- [ ] Header file created with proper includes
- [ ] Implementation file created with all methods
- [ ] Constructor initializes hardware correctly
- [ ] buildModel() parses all needed data
- [ ] render() displays data correctly
- [ ] cleanup() properly shuts down display
- [ ] Header file added to nodeapp.cpp includes
- [ ] View instantiation added to NodeApp::setup()
- [ ] Compiles without errors
- [ ] Tested on hardware
- [ ] Documentation updated

## Example: Complete Minimal LCD Implementation

File: `src/views/lcd_view.h`
```cpp
#pragma once
#include "display_view.h"
#include "model.h"

class LCDView : public DisplayView {
 private:
  DisplayModel model_;
  const JsonDocument* doc_;
  
 public:
  LCDView();
  ~LCDView() override;
  bool buildModel(const JsonDocument* doc,
                 const std::map<std::string, Sensor*>& sensors) override;
  void render() override;
  bool needsRefresh() const override { return true; }
  void cleanup() override {}
};
```

File: `src/views/lcd_view.cpp`
```cpp
#include "lcd_view.h"

LCDView::LCDView() : doc_(nullptr) {
  // Initialize I2C LCD
  Wire.begin();
}

LCDView::~LCDView() {}

bool LCDView::buildModel(const JsonDocument* doc,
                        const std::map<std::string, Sensor*>& sensors) {
  doc_ = doc;
  if (!doc || doc->isNull()) return true;
  
  model_.setDateTime("Loading...");
  return true;
}

void LCDView::render() {
  // Your LCD rendering code here
  Serial.println("Rendering on LCD");
}
```

That's it! You now have a basic display view. Expand the `render()` method to show all the data you want.
