#include <vector>

#include "epd_view_2.h"

#include "moon_phases_48pt.h"
#include "config.h"
#include "version.h"

EPDView2::EPDView2()
    : display_(nullptr),
      u8g2_(),
      previous_model_(),
      has_previous_state_(false),
      partial_update_count_(0) {}

EPDView2::~EPDView2() { cleanup(); }

void EPDView2::cleanup() {
  if (display_ != nullptr) {
    display_->hibernate();
    delete display_;
    display_ = nullptr;
  }
}

bool EPDView2::render(JsonDocument* doc,
                      const std::map<std::string, Sensor*>& sensors) {
  buildModel(doc, sensors);

  // First render or invalid data - full refresh
  if (!has_previous_state_ || !doc_is_valid_) {
    Serial.println("First render or invalid data - performing full refresh");
    has_previous_state_ = true;
    previous_model_ = model_;
    partial_update_count_ = 0;
    return fullRender();
  }

  // Force full refresh periodically to prevent ghosting
  if (partial_update_count_ >= MAX_PARTIAL_UPDATES) {
    Serial.printf("Max partial updates (%d) reached - forcing full refresh\n",
                  MAX_PARTIAL_UPDATES);
    previous_model_ = model_;
    partial_update_count_ = 0;
    return fullRender();
  }

  // Try partial updates
  if (performPartialUpdates()) {
    Serial.println("Partial updates completed successfully");
    previous_model_ = model_;
    partial_update_count_++;
    return false;  // No deep sleep needed for partial updates
  }

  // Fall back to full render if partial updates failed
  Serial.println(
      "Partial updates failed or not applicable - performing full refresh");
  previous_model_ = model_;
  partial_update_count_ = 0;
  return fullRender();
}

bool EPDView2::performPartialUpdates() {
  if (display_ == nullptr) {
    Serial.println("Display not initialized for partial updates");
    return false;
  }

  bool updated = false;
  RenderContext ctx;
  ctx.display = display_;
  ctx.u8g2 = &u8g2_;
  ctx.display_width = display_->width();
  ctx.display_height = display_->height();
  ctx.node_count = model_.getNodeData().size();
  ctx.is_partial = true;

  // Check for layout changes (node count changed)
  if (previous_model_.getNodeData().size() != model_.getNodeData().size()) {
    Serial.println("Node count changed - need full refresh");
    return false;
  }

#ifdef DISPLAY_TIME
  // Update time if changed
  if (hasTimeChanged()) {
    Serial.println("Time changed, partial update");
    ctx.mode = RenderMode::PARTIAL_TIME;
    displayTime(ctx);
    updated = true;
  }
#endif

  // Update date if changed
  if (hasDateChanged()) {
    Serial.println("Date changed, partial update");
    ctx.mode = RenderMode::PARTIAL_DATE;
    displayDate(ctx);
    updated = true;
  }

  // Update sun/moon if changed
  if (haveSunMoonChanged()) {
    Serial.println("Sun/Moon changed, partial update");
    ctx.mode = RenderMode::PARTIAL_SUN_MOON;
    displaySunAndMoon(ctx);
    updated = true;
  }

  // Update nodes if changed
  if (haveNodesChanged()) {
    Serial.println("Nodes changed, partial update");
    ctx.mode = RenderMode::PARTIAL_NODES;
    displayNodes(ctx);
    updated = true;
  }

  return updated;
}

// Returns true if full display re-initialisation is needed on next cycle
bool EPDView2::fullRender() {
  bool fullWindowRefresh = true;
  if (display_ == nullptr) {
    display_ = new GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>(
        GxEPD2_750_T7(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));
    (*display_).init(115200);
    Serial.println("E-Paper display initialized");
    u8g2_.begin(*display_);
  } else {
    Serial.println("E-Paper display previously initialized");
    fullWindowRefresh = false;
  }

  bool deepSleepNeeded = fullRenderInternal(fullWindowRefresh);
  Serial.println("E-Paper full render completed");
  return deepSleepNeeded;
}

bool EPDView2::fullRenderInternal(bool fullWindowRefresh) {
  bool deepSleepNeeded = false;

  if (fullWindowRefresh || doc_is_valid_ == false) {
    Serial.println("Performing full window refresh");
    (*display_).setFullWindow();
  } else {
    Serial.println("Performing partial window refresh");
    (*display_).setPartialWindow(0, 0, display_->width(), display_->height());
  }

  // Create RenderContext for full render
  RenderContext ctx;
  ctx.mode = RenderMode::FULL;
  ctx.display = display_;
  ctx.u8g2 = &u8g2_;
  ctx.display_width = display_->width();
  ctx.display_height = display_->height();
  ctx.node_count = model_.getNodeData().size();
  ctx.is_partial = false;

  (*display_).firstPage();
  do {
    (*display_).setRotation(0);
    (*display_).setTextColor(GxEPD_BLACK);

    u8g2_.setFontMode(0);
    u8g2_.setFontDirection(0);
    u8g2_.setForegroundColor(GxEPD_BLACK);
    u8g2_.setBackgroundColor(GxEPD_WHITE);
    u8g2_.setFont(defaultFont);

    if (doc_is_valid_ == false) {
      u8g2_.setCursor(0, 24);
      u8g2_.println("Failed to get data - local sensor only");
      displayLocalSensorData();
      deepSleepNeeded = true;
    } else {
      uint row_offset = displayNodes(ctx);

      row_offset += font_height_spacing_24pt;
      u8g2_.setCursor(0, row_offset);
      displaySunAndMoon(ctx);

      // TODO: re-enable if we get partial updates working reliably
      // Serial.printf("Time: %s\n", model_.getTime().c_str());
#ifdef DISPLAY_TIME
      displayTime(ctx);
#endif

      displayDate(ctx);
    }
  } while ((*display_).nextPage());

  // TODO: remove if we succeed in getting partial updates working reliably
#ifdef FORCE_DEEP_SLEEP
  Serial.println(F("Forcing deep sleep after full render"));
  deepSleepNeeded = true;
#endif

  return deepSleepNeeded;
}

bool EPDView2::partialRender() {
  if (display_ == nullptr) {
    Serial.println("E-Paper display not initialized for partial render");
    return false;
  }

  // partialRenderInternal();
  fullRenderInternal(false);
  Serial.println("E-Paper partial render completed");

  return true;
}

void EPDView2::partialRenderInternal() {
  Serial.printf("Time: %s\n", model_.getTime().c_str());
  int x = 0;
  int y = display_->height() - 10 - font_height_spacing_38pt;
  u8g2_.setFont(largeFont);
  uint str_width = u8g2_.getUTF8Width(model_.getTime().c_str());
  (*display_).setPartialWindow(x, y, str_width, font_height_spacing_38pt);
  (*display_).firstPage();
  do {
    u8g2_.setFontMode(0);
    u8g2_.setFontDirection(0);
    u8g2_.setForegroundColor(GxEPD_BLACK);
    u8g2_.setBackgroundColor(GxEPD_WHITE);

    u8g2_.setCursor(0, display_->height() - 10);
    u8g2_.printf("%s", model_.getTime().c_str());
  } while ((*display_).nextPage());
}

void EPDView2::displayLocalSensorData() {
  if (sensors_.find("bme680") == sensors_.end() || !sensors_["bme680"]->ok()) {
    u8g2_.println("Local sensor (BME680) setup failed\n");
  } else {
    std::map<std::string, Measurement> measurements =
        sensors_["bme680"]->read();
    for (const auto& measurement : measurements) {
      u8g2_.printf("%s: ", measurement.first.c_str());
      Measurement m = measurement.second;
      u8g2_.printf("%.2f %s\n\n", m.value, m.unit.c_str());
    }
  }
}

void EPDView2::displaySunAndMoon(const RenderContext& ctx) {
  if (ctx.is_partial) {
    // Calculate sun/moon display region
    // For partial updates, we need to know where nodes end
    // This is a simplified approach - in full render, this is called after
    // displayNodes()
    u8g2_.setFont(defaultFont);
    int height = font_height_spacing_24pt * 2 + 48;  // 2 lines + moon icon

    // Calculate starting Y position (after nodes area)
    // In full render, this is passed as row_offset from displayNodes()
    // For partial, we approximate based on layout
    int node_area_height = ctx.display_height - font_height_spacing_38pt * 2;
    int y = node_area_height;

    Serial.printf("displaySunAndMoon partial: window (0,%d) size (%dx%d)\n", y,
                  ctx.display_width, height);
    display_->setPartialWindow(0, y, ctx.display_width, height);
    display_->firstPage();
  }

  do {
    if (ctx.is_partial) {
      display_->fillScreen(GxEPD_WHITE);
      u8g2_.setFontMode(0);
      u8g2_.setFontDirection(0);
      u8g2_.setForegroundColor(GxEPD_BLACK);
      u8g2_.setBackgroundColor(GxEPD_WHITE);
      u8g2_.setFont(defaultFont);

      // Set cursor to top of partial window
      u8g2_.setCursor(0, font_height_spacing_24pt);
    }

    u8g2_.printf("Sun:  %s  %s  %s\n", model_.getSunRise().c_str(),
                 model_.getSunTransit().c_str(), model_.getSunSet().c_str());
    u8g2_.printf("Moon: %s  %s  %s  ", model_.getMoonRise().c_str(),
                 model_.getMoonTransit().c_str(), model_.getMoonSet().c_str());

    u8g2_.setFont(moon_phases_48pt);
    u8g2_.print(model_.getMoonPhaseLetter());
    u8g2_.setFont(defaultFont);
  } while (ctx.is_partial && display_->nextPage());
}

uint EPDView2::displayNodes(const RenderContext& ctx) {
  if (ctx.is_partial && ctx.mode == RenderMode::PARTIAL_NODES) {
    // Calculate nodes region
    int height = ctx.display_height - font_height_spacing_38pt * 2;

    Serial.printf("displayNodes partial: window (0,0) size (%dx%d)\n",
                  ctx.display_width, height);
    display_->setPartialWindow(0, 0, ctx.display_width, height);
    display_->firstPage();
  }

  uint max_row_offset = 0;

  do {
    if (ctx.is_partial && ctx.mode == RenderMode::PARTIAL_NODES) {
      display_->fillScreen(GxEPD_WHITE);
      u8g2_.setFontMode(0);
      u8g2_.setFontDirection(0);
      u8g2_.setForegroundColor(GxEPD_BLACK);
      u8g2_.setBackgroundColor(GxEPD_WHITE);
      u8g2_.setFont(defaultFont);
    }

    JsonObject nodes = model_.getNodeData();
    int column = 0;
    max_row_offset = 0;

    for (JsonPair node : nodes) {
      uint8_t row = 1;
      uint row_offset = 0;
      JsonObject nodeData = node.value().as<JsonObject>();
      displayNodeHeader(node, nodeData, ctx, column, row, row_offset);
      displayNodeMeasurements(nodeData, ctx, column, row, row_offset);
      // displayBatteryLevel(nodeData, ctx.node_count, column, row, row_offset);
      displayBadStatuses(nodeData, ctx.node_count, column, row, row_offset);
      displayStaleState(nodeData, ctx.node_count, column, row, row_offset);
      displayNodeVersion(nodeData, ctx.node_count, column, row, row_offset);

      column++;
      if (row_offset > max_row_offset) {
        max_row_offset = row_offset;
      }
    }
  } while (ctx.is_partial && ctx.mode == RenderMode::PARTIAL_NODES &&
           display_->nextPage());

  return max_row_offset;
}

void EPDView2::displayNodeHeader(JsonPair& node, JsonObject& nodeData,
                                 const RenderContext& ctx, int column,
                                 uint8_t& row, uint& row_offset) {
  JsonObject node_data = model_.getNodeData()[node.key()].as<JsonObject>();

  std::string display_name = node_data["display_name"].as<String>().c_str();
  int column_width = ctx.display_width / ctx.node_count;
  row_offset = row * font_height_spacing_24pt;
  row++;
  u8g2_.setCursor(column * column_width, row_offset);
  u8g2_.printf("%s ", display_name.c_str());
  displayBatteryLevel(nodeData);

  // Leave an empty half row after header
  row_offset += font_height_spacing_24pt / 2;
  row++;
}

void EPDView2::displayBadStatuses(JsonObject& nodeData, int node_count,
                                  int column, uint8_t& row, uint& row_offset) {
  int column_width = display_->width() / node_count;
  int row_height = font_height_spacing_16pt;

  u8g2_.setFont(smallFont);

  if (nodeData["status"].is<JsonObject>()) {
    JsonObject status = nodeData["status"].as<JsonObject>();
    for (JsonPair kvp : status) {
      String value = kvp.value().as<String>();
      if (value != "ok") {
        row_offset += row_height;
        row++;
        u8g2_.setCursor(column * column_width, row_offset);
        String key = kvp.key().c_str();
        u8g2_.printf("%s:%s", key.c_str(), value.c_str());
      }
    }
  }

  u8g2_.setFont(defaultFont);
}

void EPDView2::displayStaleState(JsonObject& nodeData, int node_count,
                                 int column, uint8_t& row, uint& row_offset) {
  int column_width = display_->width() / node_count;
  int row_height = font_height_spacing_16pt;

  u8g2_.setFont(smallFont);

  row_offset += row_height;
  row++;
  u8g2_.setCursor(column * column_width, row_offset);

  std::string node_stale = nodeData["stale_state"].as<String>().c_str();
  if (!node_stale.empty()) {
    u8g2_.printf("%s", node_stale.c_str());
  }

  u8g2_.setFont(defaultFont);
}

void EPDView2::displayNodeVersion(JsonObject& nodeData, int node_count,
                                  int column, uint8_t& row, uint& row_offset) {
#ifdef DISPLAY_NODE_VERSIONS
  if (!nodeData["version"].is<JsonString>()) {
    return;
  }

  int column_width = display_->width() / node_count;
  int row_height = font_height_spacing_16pt;

  u8g2_.setFont(smallFont);

  row_offset += row_height;
  row++;
  u8g2_.setCursor(column * column_width, row_offset);

  std::string version = nodeData["version"].as<String>().c_str();
  // Display only first 8 characters of the git SHA1 hash
  if (version.length() > 8) {
    version = version.substr(0, 8);
  }
  u8g2_.printf("v:%s", version.c_str());

  u8g2_.setFont(defaultFont);
#endif
}

void EPDView2::displayNodeMeasurements(JsonObject& nodeData,
                                       const RenderContext& ctx, int column,
                                       uint8_t& row, uint& row_offset) {
  if (nodeData["measurements_v2"].is<JsonObject>()) {
    JsonObject measurements_v2 = nodeData["measurements_v2"].as<JsonObject>();
    std::vector<std::string> devices = {"bme680", "sht31d"};
    for (const auto& device : devices) {
      displayDeviceMeasurements(measurements_v2, device, nodeData,
                                ctx.node_count, column, row, row_offset);
    }
  }
}

void EPDView2::displayDeviceMeasurements(JsonObject& measurements_v2,
                                         const std::string& device,
                                         JsonObject& nodeData, int node_count,
                                         int column, uint8_t& row,
                                         uint& row_offset) {
  int column_width = display_->width() / node_count;

  if (measurements_v2[device].is<JsonObject>()) {
    JsonObject device_map = measurements_v2[device].as<JsonObject>();
    if (device_map["temperature"].is<JsonVariant>()) {
      auto min_max = getDeviceMinMax(nodeData, device, "temperature");
      if (min_max.first) {
        u8g2_.setFont(smallFont);
        row_offset += font_height_spacing_16pt;
        row++;
        u8g2_.setCursor(column * column_width, row_offset);
        u8g2_.printf("%.1f°C %.1f°C", min_max.second.first,
                     min_max.second.second);
        u8g2_.setFont(defaultFont);
      }

      u8g2_.setFont(largeFont);
      row_offset += font_height_spacing_38pt;
      row++;
      u8g2_.setCursor(column * column_width, row_offset);
      u8g2_.printf("%.1f°C", float(device_map["temperature"]));
      u8g2_.setFont(defaultFont);
    }
    if (device_map["humidity"].is<JsonVariant>()) {
      auto min_max = getDeviceMinMax(nodeData, device, "humidity");
      if (min_max.first) {
        u8g2_.setFont(smallFont);
        row_offset += font_height_spacing_16pt;
        row++;
        u8g2_.setCursor(column * column_width, row_offset);
        u8g2_.printf("%.1f%% %.1f%%", min_max.second.first,
                     min_max.second.second);
        u8g2_.setFont(defaultFont);
      }

      u8g2_.setFont(largeFont);
      row_offset += font_height_spacing_38pt;
      row++;
      u8g2_.setCursor(column * column_width, row_offset);
      u8g2_.printf("%.1f%%", float(device_map["humidity"]));
      u8g2_.setFont(defaultFont);
    }
    if (device_map["pressure"].is<JsonVariant>()) {
      auto min_max = getDeviceMinMax(nodeData, device, "pressure");
      if (min_max.first) {
        u8g2_.setFont(smallFont);
        row_offset += font_height_spacing_16pt;
        row++;
        u8g2_.setCursor(column * column_width, row_offset);
        u8g2_.printf("%.0fhPa %.0fhPa", min_max.second.first,
                     min_max.second.second);
        u8g2_.setFont(defaultFont);
      }

      u8g2_.setFont(largeFont);
      row_offset += font_height_spacing_38pt;
      row++;
      u8g2_.setCursor(column * column_width, row_offset);
      u8g2_.printf("%.0fhPa ", float(device_map["pressure"]));
      u8g2_.setFont(defaultFont);
    }
  }
}

std::pair<bool, std::pair<float, float>> EPDView2::getDeviceMinMax(
    JsonObject& nodeData, const std::string& device,
    const std::string& measurement) {
  float min = -1;
  float max = -1;
  bool found = false;

  if (nodeData["measurements_min_max"].is<JsonObject>()) {
    JsonObject min_max = nodeData["measurements_min_max"].as<JsonObject>();
    if (min_max[device].is<JsonObject>()) {
      JsonObject device_min_max = min_max[device].as<JsonObject>();
      if (device_min_max[measurement].is<JsonObject>()) {
        JsonObject measurement_min_max =
            device_min_max[measurement].as<JsonObject>();
        min = float(measurement_min_max["min"]);
        max = float(measurement_min_max["max"]);
        found = true;
      }
    }
  }

  return std::make_pair(found, std::make_pair(min, max));
}

void EPDView2::displayBatteryLevel(JsonObject& nodeData, int node_count,
                                   int column, uint8_t& row, uint& row_offset) {
  if (!nodeData["battery_level"].is<JsonString>()) {
    return;
  }

  int column_width = display_->width() / node_count;
  int row_height = font_height_spacing_24pt;
  row_offset += row_height;
  row++;

  u8g2_.setCursor(column * column_width, row_offset);

  displayBatteryLevel(nodeData);
}

void EPDView2::displayBatteryLevel(JsonObject& nodeData) {
  if (!nodeData["battery_level"].is<JsonString>()) {
    return;
  }

  std::string level = nodeData["battery_level"].as<std::string>();
  u8g2_.setFont(u8g2_font_battery24_tr);
  u8g2_.print(level.c_str());
  u8g2_.setFont(defaultFont);
}

// Change detection methods
bool EPDView2::hasTimeChanged() const {
  if (!has_previous_state_) {
    return false;
  }
  return previous_model_.getTime() != model_.getTime();
}

bool EPDView2::hasDateChanged() const {
  if (!has_previous_state_) {
    return false;
  }
  return previous_model_.getDate() != model_.getDate();
}

bool EPDView2::haveSunMoonChanged() const {
  if (!has_previous_state_) {
    return false;
  }
  return previous_model_.getSunRise() != model_.getSunRise() ||
         previous_model_.getSunSet() != model_.getSunSet() ||
         previous_model_.getSunTransit() != model_.getSunTransit() ||
         previous_model_.getMoonRise() != model_.getMoonRise() ||
         previous_model_.getMoonSet() != model_.getMoonSet() ||
         previous_model_.getMoonTransit() != model_.getMoonTransit() ||
         previous_model_.getMoonPhaseLetter() != model_.getMoonPhaseLetter();
}

bool EPDView2::haveNodesChanged() const {
  if (!has_previous_state_) {
    return false;
  }

  // TODO: implement per-node comparison for more granular updates
  return previous_model_ != model_;
}

// Display methods with RenderContext support
void EPDView2::displayTime(const RenderContext& ctx) {
  u8g2_.setFont(largeFont);
  uint str_width = u8g2_.getUTF8Width(model_.getTime().c_str());

  if (ctx.is_partial) {
    int x = 0;
    int y = ctx.display_height - 10 - font_height_spacing_38pt;
    int width = str_width + 20;  // Add padding
    int height = font_height_spacing_38pt;

    Serial.printf("displayTime partial: window (%d,%d) size (%dx%d)\n", x, y,
                  width, height);
    display_->setPartialWindow(x, y, width, height);
    display_->firstPage();
  }

  do {
    if (ctx.is_partial) {
      display_->fillScreen(GxEPD_WHITE);
      u8g2_.setFontMode(0);
      u8g2_.setFontDirection(0);
      u8g2_.setForegroundColor(GxEPD_BLACK);
      u8g2_.setBackgroundColor(GxEPD_WHITE);
      u8g2_.setFont(largeFont);
    }

    u8g2_.setCursor(0, ctx.display_height - 10);
    u8g2_.printf("%s", model_.getTime().c_str());
  } while (ctx.is_partial && display_->nextPage());

  if (!ctx.is_partial) {
    u8g2_.setFont(defaultFont);
  }
}

void EPDView2::displayDate(const RenderContext& ctx) {
  u8g2_.setFont(defaultFont);
  uint str_width = u8g2_.getUTF8Width(model_.getDate().c_str());
  int x = ctx.display_width - str_width;

  if (ctx.is_partial) {
    int y = ctx.display_height - 10 - font_height_spacing_24pt;
    int width = str_width + 20;  // Add padding
    int height = font_height_spacing_24pt;

    Serial.printf("displayDate partial: window (%d,%d) size (%dx%d)\n", x, y,
                  width, height);
    display_->setPartialWindow(x - 10, y, width, height);
    display_->firstPage();
  }

  do {
    if (ctx.is_partial) {
      display_->fillScreen(GxEPD_WHITE);
      u8g2_.setFontMode(0);
      u8g2_.setFontDirection(0);
      u8g2_.setForegroundColor(GxEPD_BLACK);
      u8g2_.setBackgroundColor(GxEPD_WHITE);
      u8g2_.setFont(defaultFont);
    }

    u8g2_.setCursor(x, ctx.display_height - 10);
    u8g2_.printf("%s", model_.getDate().c_str());
  } while (ctx.is_partial && display_->nextPage());
}
