#pragma once

#include <ArduinoJson.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <map>
#include <string>

#include "controller.h"
#include "datetime.h"
#include "display_view.h"
#include "fonts/u8g2_font_battery24_tr.h"
#include "model.h"
#include "sensor.h"

// Pin mapping for many ESP32 dev boards and Waveshare 7.5in V2 SPI displays:
#define EPD_CS 5
#define EPD_DC 17
#define EPD_RST 16
#define EPD_BUSY 4

/**
 * E-Paper Display (EPD) implementation of DisplayView.
 * Renders display data to a Waveshare 7.5" E-Paper display.
 */
class EPDView : public DisplayView {
 private:
  GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>* display_;
  U8G2_FOR_ADAFRUIT_GFX u8g2_;
  const uint8_t* defaultFont = u8g2_font_inb24_mf;
  DisplayModel model_;
  DateTime utc_timestamp_;
  DateTime local_timestamp_;
  const JsonDocument* doc_;
  std::map<std::string, Sensor*> sensors_;
  bool needs_refresh_;

  // Helper methods for rendering
  void displaySunAndMoon();
  void displayNodes();
  void displayNodeHeader(JsonPair& node, JsonObject& nodeData);
  void displayNodeMeasurements(JsonObject& nodeData);
  void displayDeviceMeasurements(JsonObject& measurements_v2,
                                 const std::string& device,
                                 JsonObject& nodeData);
  void displayBatteryLevel(JsonString level);
  void displayBadStatuses(JsonObject& nodeData);
  std::pair<bool, std::pair<float, float>> getDeviceMinMax(
      JsonObject& nodeData, const std::string& device,
      const std::string& measurement);
  DateTime parseTimestampValue(const String& timestamp_key);
  DateTime parseTimestamp(const std::string& timestamp,
                          const String& timestamp_key);
  void calculateSunAndMoon();
  void displayLocalSensorData();

 public:
  EPDView();
  ~EPDView() override;

  bool buildModel(const JsonDocument* doc,
                  const std::map<std::string, Sensor*>& sensors) override;
  void render() override;
  bool needsRefresh() const override { return needs_refresh_; }
  void cleanup() override;
};
