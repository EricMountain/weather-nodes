#include <vector>

#include "epd_view.h"

#include "fonts/moon_phases_48pt.h"

EPDView::EPDView()
    : display_(nullptr), u8g2_(), model_(), needs_refresh_(true) {}

EPDView::~EPDView() { cleanup(); }

void EPDView::cleanup() {
  if (display_ != nullptr) {
    display_->hibernate();
    delete display_;
    display_ = nullptr;
  }
}

bool EPDView::buildModel(JsonDocument* doc,
                         const std::map<std::string, Sensor*>& sensors) {
  doc_ = doc;
  sensors_ = sensors;

  // Force refresh if we have no data
  if (doc_ == nullptr || doc_->isNull() || !(*doc_)["nodes"].is<JsonObject>()) {
    needs_refresh_ = true;
    return true;
  }

  utc_timestamp_ = parseTimestampValue("timestamp_utc");
  local_timestamp_ = parseTimestampValue("timestamp_local");

  model_.buildFromJson(doc_, utc_timestamp_, local_timestamp_);

  Controller c = Controller(model_);
  needs_refresh_ = c.needRefresh();
  return needs_refresh_;
}

void EPDView::render() {
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
      u8g2_.printf("%s  ", model_.getDateTime().c_str());
      u8g2_.println();
      u8g2_.println();

      displaySunAndMoon();
      u8g2_.println();

      displayNodes();
    }
  } while ((*display_).nextPage());
}

void EPDView::displayLocalSensorData() {
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

void EPDView::displaySunAndMoon() {
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

void EPDView::displayNodes() {
  JsonObject nodes = (*doc_)["nodes"];
  for (JsonPair node : nodes) {
    JsonObject nodeData = node.value().as<JsonObject>();
    displayNodeHeader(node, nodeData);
    displayNodeMeasurements(nodeData);
    u8g2_.println();
  }
}

void EPDView::displayNodeHeader(JsonPair& node, JsonObject& nodeData) {
  JsonObject node_data = model_.getNodeData()[node.key()].as<JsonObject>();

  std::string display_name = node_data["display_name"].as<String>().c_str();
  u8g2_.printf("%s", display_name.c_str());

  displayBatteryLevel(node_data["battery_level"].as<JsonString>());
  displayBadStatuses(nodeData);

  std::string node_stale = node_data["stale_state"].as<String>().c_str();
  if (node_stale != "") {
    u8g2_.printf(" %s", node_stale.c_str());
  }
  u8g2_.println();
}

void EPDView::displayBadStatuses(JsonObject& nodeData) {
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

void EPDView::displayNodeMeasurements(JsonObject& nodeData) {
  if (nodeData["measurements_v2"].is<JsonObject>()) {
    JsonObject measurements_v2 = nodeData["measurements_v2"].as<JsonObject>();
    std::vector<std::string> devices = {"bme680", "sht31d"};
    for (const auto& device : devices) {
      displayDeviceMeasurements(measurements_v2, device, nodeData);
    }
  }
}

void EPDView::displayDeviceMeasurements(JsonObject& measurements_v2,
                                        const std::string& device,
                                        JsonObject& nodeData) {
  if (measurements_v2[device].is<JsonObject>()) {
    JsonObject device_map = measurements_v2[device].as<JsonObject>();
    if (device_map["temperature"].is<JsonString>()) {
      auto min_max = getDeviceMinMax(nodeData, device, "temperature");
      if (min_max.first) {
        u8g2_.printf(" %.1f(%.1f/%.1f)°C ", float(device_map["temperature"]),
                     min_max.second.first, min_max.second.second);
      } else {
        u8g2_.printf(" %.1f°C", float(device_map["temperature"]));
      }
    }
    if (device_map["humidity"].is<JsonString>()) {
      u8g2_.printf(" %.1f%% ", float(device_map["humidity"]));
    }
    if (device_map["pressure"].is<JsonString>()) {
      u8g2_.printf(" %.0fhPa ", float(device_map["pressure"]));
    }
    u8g2_.println();
  }
}

std::pair<bool, std::pair<float, float>> EPDView::getDeviceMinMax(
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
      }
      found = true;
    }
  }

  return std::make_pair(found, std::make_pair(min, max));
}

void EPDView::displayBatteryLevel(JsonString level) {
  u8g2_.print(" ");
  u8g2_.setFont(u8g2_font_battery24_tr);
  u8g2_.print(level.c_str());
  u8g2_.setFont(defaultFont);
}
