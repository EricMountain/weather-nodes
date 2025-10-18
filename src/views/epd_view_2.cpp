#include <vector>

#include "epd_view_2.h"

#include "fonts/moon_phases_48pt.h"

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
      // u8g2_.printf("%s  ", model_.getDateTime().c_str());
      // u8g2_.println();
      // u8g2_.println();

      // displaySunAndMoon();
      // u8g2_.println();

      displayNodes();
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
  u8g2_.println();

  u8g2_.printf("      %s\n", model_.getMoonPhase().c_str());
}

void EPDView2::displayNodes() {
  JsonObject nodes = model_.getNodeData();
  int column = 0;
  int node_count = nodes.size();
  for (JsonPair node : nodes) {
    uint8_t row = 1;
    JsonObject nodeData = node.value().as<JsonObject>();
    displayNodeHeader(node, nodeData, node_count, column, row);
    displayNodeMeasurements(nodeData, node_count, column, row);
    // u8g2_.println();
    column++;
  }
}

void EPDView2::displayNodeHeader(JsonPair& node, JsonObject& nodeData,
                                 int node_count, int column, uint8_t& row) {
  JsonObject node_data = model_.getNodeData()[node.key()].as<JsonObject>();

  std::string display_name = node_data["display_name"].as<String>().c_str();
  int column_width = display_->width() / node_count;
  u8g2_.setCursor(column * column_width, row * font_height_spacing_24pt);
  u8g2_.printf("%s", display_name.c_str());
  row++;

  // displayBatteryLevel(node_data["battery_level"].as<JsonString>());
  // displayBadStatuses(nodeData);

  // std::string node_stale = node_data["stale_state"].as<String>().c_str();
  // if (node_stale != "") {
  //   u8g2_.printf(" %s", node_stale.c_str());
  // }
  // u8g2_.println();
}

void EPDView2::displayBadStatuses(JsonObject& nodeData) {
  // Display status entries that are not "ok"
  if (nodeData["status"].is<JsonObject>()) {
    JsonObject status = nodeData["status"].as<JsonObject>();
    for (JsonPair kvp : status) {
      String value = kvp.value().as<String>();
      if (value != "ok") {
        String key = kvp.key().c_str();
        u8g2_.printf(" %s=%s", key.c_str(), value.c_str());
      }
    }
  }
}

void EPDView2::displayNodeMeasurements(JsonObject& nodeData, int node_count,
                                       int column, uint8_t& row) {
  if (nodeData["measurements_v2"].is<JsonObject>()) {
    Serial.println("Found measurements_v2");
    JsonObject measurements_v2 = nodeData["measurements_v2"].as<JsonObject>();
    std::vector<std::string> devices = {"bme680", "sht31d"};
    for (const auto& device : devices) {
      displayDeviceMeasurements(measurements_v2, device, nodeData, node_count,
                                column, row);
    }
  } else {
    Serial.println("No measurements_v2 found");
  }
}

void EPDView2::displayDeviceMeasurements(JsonObject& measurements_v2,
                                         const std::string& device,
                                         JsonObject& nodeData, int node_count,
                                         int column, uint8_t& row) {
  int column_width = display_->width() / node_count;
  int row_height = font_height_spacing_24pt;

  if (measurements_v2[device].is<JsonObject>()) {
    Serial.printf("Displaying measurements for device: %s\n", device.c_str());
    JsonObject device_map = measurements_v2[device].as<JsonObject>();
    if (device_map["temperature"].is<JsonVariant>()) {
      auto min_max = getDeviceMinMax(nodeData, device, "temperature");
      Serial.printf("Cursor set to (%d, %d)\n", column * column_width,
                    row * row_height);
      u8g2_.setCursor(column * column_width, (row++) * row_height);
      if (min_max.first) {
        u8g2_.printf(" %.1f(%.1f/%.1f)°C ", float(device_map["temperature"]),
                     min_max.second.first, min_max.second.second);
      } else {
        u8g2_.printf(" %.1f°C", float(device_map["temperature"]));
      }
    }
    if (device_map["humidity"].is<JsonVariant>()) {
      Serial.printf("Cursor set to (%d, %d)\n", column * column_width,
                    row * row_height);
      u8g2_.setCursor(column * column_width, (row++) * row_height);
      u8g2_.printf(" %.1f%% ", float(device_map["humidity"]));
    }
    if (device_map["pressure"].is<JsonVariant>()) {
      Serial.printf("Cursor set to (%d, %d)\n", column * column_width,
                    row * row_height);
      u8g2_.setCursor(column * column_width, (row++) * row_height);
      u8g2_.printf(" %.0fhPa ", float(device_map["pressure"]));
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

void EPDView2::displayBatteryLevel(JsonString level) {
  u8g2_.print(" ");
  u8g2_.setFont(u8g2_font_battery24_tr);
  u8g2_.print(level.c_str());
  u8g2_.setFont(defaultFont);
}
