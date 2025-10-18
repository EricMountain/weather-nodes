#include <vector>

#include "epd_view_2.h"

#include "fonts/moon_phases_48pt.h"
#include "version.h"

EPDView2::EPDView2() : display_(nullptr), u8g2_() {}

EPDView2::~EPDView2() { cleanup(); }

void EPDView2::cleanup() {
  if (display_ != nullptr) {
    display_->hibernate();
    delete display_;
    display_ = nullptr;
  }
}

void EPDView2::render() {
  if (display_ == nullptr) {
    display_ = new GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>(
        GxEPD2_750_T7(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

    (*display_).init(115200);
    Serial.println("E-Paper display initialized");
    (*display_).setRotation(0);  // Landscape
    u8g2_.begin(*display_);
  }

  (*display_).setFullWindow();
  (*display_).firstPage();
  do {
    u8g2_.setFontMode(0);
    u8g2_.setFontDirection(0);
    u8g2_.setForegroundColor(GxEPD_BLACK);
    u8g2_.setBackgroundColor(GxEPD_WHITE);
    u8g2_.setFont(defaultFont);
    u8g2_.setCursor(0, 24);

    // TODO: need to distinguish GET failure vs partial response
    if (doc_ == nullptr || doc_->isNull() ||
        !(*doc_)["nodes"].is<JsonObject>()) {
      u8g2_.println("Failed to get data - local sensor only");
      displayLocalSensorData();
    } else {
      uint row_offset = displayNodes();

      row_offset += font_height_spacing_24pt;
      u8g2_.setCursor(0, row_offset);
      displaySunAndMoon();

      row_offset += font_height_spacing_24pt * 3;
      u8g2_.setCursor(0, row_offset);
      u8g2_.printf("%s  ", model_.getDateTime().c_str());
    }
  } while ((*display_).nextPage());
}

void EPDView2::displayLocalSensorData() {
  if (sensors_.find("bme680") == sensors_.end() || !sensors_["bme680"]->ok()) {
    u8g2_.println("Local sensor (BME680) setup failed");
  } else {
    std::map<std::string, Measurement> measurements =
        sensors_["bme680"]->read();
    for (const auto& measurement : measurements) {
      u8g2_.printf("%s: ", measurement.first.c_str());
      Measurement m = measurement.second;
      u8g2_.printf("%.2f %s\n", m.value, m.unit.c_str());
    }
  }
}

void EPDView2::displaySunAndMoon() {
  u8g2_.printf("Sun:  %s  %s  %s\n", model_.getSunRise().c_str(),
               model_.getSunTransit().c_str(), model_.getSunSet().c_str());
  u8g2_.printf("Moon: %s  %s  %s  ", model_.getMoonRise().c_str(),
               model_.getMoonTransit().c_str(), model_.getMoonSet().c_str());

  u8g2_.setFont(moon_phases_48pt);
  u8g2_.print(model_.getMoonPhaseLetter());
  u8g2_.setFont(defaultFont);
}

uint EPDView2::displayNodes() {
  JsonObject nodes = model_.getNodeData();
  int column = 0;
  int node_count = nodes.size();
  uint max_row_offset = 0;
  for (JsonPair node : nodes) {
    uint8_t row = 1;
    uint row_offset = 0;
    JsonObject nodeData = node.value().as<JsonObject>();
    displayNodeHeader(node, nodeData, node_count, column, row, row_offset);
    displayNodeMeasurements(nodeData, node_count, column, row, row_offset);
    displayBatteryLevel(nodeData, node_count, column, row, row_offset);
    displayBadStatuses(nodeData, node_count, column, row, row_offset);
    displayStaleState(nodeData, node_count, column, row, row_offset);

    column++;
    if (row_offset > max_row_offset) {
      max_row_offset = row_offset;
    }
  }

  return max_row_offset;
}

void EPDView2::displayNodeHeader(JsonPair& node, JsonObject& nodeData,
                                 int node_count, int column, uint8_t& row,
                                 uint& row_offset) {
  JsonObject node_data = model_.getNodeData()[node.key()].as<JsonObject>();

  std::string display_name = node_data["display_name"].as<String>().c_str();
  int column_width = display_->width() / node_count;
  row_offset = row * font_height_spacing_24pt;
  row++;
  u8g2_.setCursor(column * column_width, row_offset);
  u8g2_.printf("%s", display_name.c_str());

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

  // Display HTTP POST error code as a special case
  if (http_post_error_code_ != 0) {
    row_offset += row_height;
    row++;
    u8g2_.setCursor(column * column_width, row_offset);
    u8g2_.printf("http_post:error_%d", http_post_error_code_);
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
    u8g2_.printf(" %s", node_stale.c_str());
  }

  u8g2_.setFont(defaultFont);
}

void EPDView2::displayNodeMeasurements(JsonObject& nodeData, int node_count,
                                       int column, uint8_t& row,
                                       uint& row_offset) {
  if (nodeData["measurements_v2"].is<JsonObject>()) {
    JsonObject measurements_v2 = nodeData["measurements_v2"].as<JsonObject>();
    std::vector<std::string> devices = {"bme680", "sht31d"};
    for (const auto& device : devices) {
      displayDeviceMeasurements(measurements_v2, device, nodeData, node_count,
                                column, row, row_offset);
    }
  }
}

void EPDView2::displayDeviceMeasurements(JsonObject& measurements_v2,
                                         const std::string& device,
                                         JsonObject& nodeData, int node_count,
                                         int column, uint8_t& row,
                                         uint& row_offset) {
  int column_width = display_->width() / node_count;
  int row_height = font_height_spacing_24pt;

  if (measurements_v2[device].is<JsonObject>()) {
    JsonObject device_map = measurements_v2[device].as<JsonObject>();
    if (device_map["temperature"].is<JsonVariant>()) {
      auto min_max = getDeviceMinMax(nodeData, device, "temperature");
      if (min_max.first) {
        u8g2_.setFont(smallFont);
        row_offset += font_height_spacing_16pt;
        row++;
        u8g2_.setCursor(column * column_width, row_offset);
        u8g2_.printf("%.1f°C ", min_max.second.first);
        u8g2_.setFont(defaultFont);
      }
      row_offset += row_height;
      row++;
      u8g2_.setCursor(column * column_width, row_offset);
      u8g2_.printf("%.1f°C", float(device_map["temperature"]));
      if (min_max.first) {
        u8g2_.setFont(smallFont);
        row_offset += font_height_spacing_16pt;
        row++;
        u8g2_.setCursor(column * column_width, row_offset);
        u8g2_.printf("%.1f°C ", min_max.second.second);
        u8g2_.setFont(defaultFont);
      }
    }
    if (device_map["humidity"].is<JsonVariant>()) {
      row_offset += row_height;
      row++;
      u8g2_.setCursor(column * column_width, row_offset);
      u8g2_.printf("%.1f%% ", float(device_map["humidity"]));
    }
    if (device_map["pressure"].is<JsonVariant>()) {
      row_offset += row_height;
      row++;
      u8g2_.setCursor(column * column_width, row_offset);
      u8g2_.printf("%.0fhPa ", float(device_map["pressure"]));
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

  std::string level = nodeData["battery_level"].as<JsonString>().c_str();

  u8g2_.setCursor(column * column_width, row_offset);
  u8g2_.setFont(u8g2_font_battery24_tr);
  u8g2_.print(level.c_str());
  u8g2_.setFont(defaultFont);
}
