#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <map>
#include <string>

// #include "controller.h"
#include "datetime.h"
#include "display_view.h"
#include "u8g2_font_battery24_tr.h"
#include "model.h"
#include "sensor.h"
#include "config.h"

// Pin mapping for many ESP32 dev boards and Waveshare 7.5in V2 SPI displays:
#ifdef USE_THINGPULSE_EPULSE_FEATHER
// Wiring for ThingPulse ePulse Feather
#define EPD_CS 15
#define EPD_DC 27
#define EPD_RST 26
#define EPD_BUSY 25
#else
// Wiring for Waveshare ESP32
#define EPD_CS 5
#define EPD_DC 17
#define EPD_RST 16
#define EPD_BUSY 4
#endif

#define DISPLAY_WIDTH GxEPD2_750_T7::WIDTH_VISIBLE
#define DISPLAY_HEIGHT GxEPD2_750_T7::HEIGHT

/**
 * E-Paper Display (EPD) implementation of DisplayView.
 * Renders display data to a Waveshare 7.5" E-Paper display.
 */
class EPDView2 : public DisplayView {
 private:
  enum class RenderMode {
    FULL,
    PARTIAL_TIME,
    PARTIAL_DATE,
    PARTIAL_NODES,
    PARTIAL_SUN_MOON
  };

  struct RenderContext {
    RenderMode mode;
    GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>* display;
    U8G2_FOR_ADAFRUIT_GFX* u8g2;
    uint16_t display_width;
    uint16_t display_height;
    int node_count;
    bool is_partial;
  };

  GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>* display_;
  U8G2_FOR_ADAFRUIT_GFX u8g2_;

  // State tracking for partial updates
  Model previous_model_;
  bool has_previous_state_;
  uint8_t partial_update_count_;
  static constexpr uint8_t MAX_PARTIAL_UPDATES = 10;

  // Font list and metrics:
  // https://github.com/olikraus/u8g2/wiki/fntlistall
  // https://digitalaccessibility.virginia.edu/accessibility-font-size-conversions

  // 38pt fonts
  const uint8_t* largeFont = u8g2_font_inb38_mf;
  static const uint8_t font_height_spacing_38pt = 50;

  // 24pt fonts
  const uint8_t* defaultFont = u8g2_font_inb24_mf;
  const uint8_t* batteryFont = u8g2_font_battery24_tr;
  static const uint8_t font_height_spacing_24pt = 32 + 2;

  // 16pt fonts
  const uint8_t* smallFont = u8g2_font_inb16_mf;
  static const uint8_t font_height_spacing_16pt = 22 + 6;

  // Change detection methods
  bool hasTimeChanged() const;
  bool hasDateChanged() const;
  bool haveSunMoonChanged() const;
  bool haveNodesChanged() const;

  // Partial update orchestration
  bool performPartialUpdates();

  // Display methods with RenderContext support
  void displayTime(const RenderContext& ctx);
  void displayDate(const RenderContext& ctx);
  void displaySunAndMoon(const RenderContext& ctx);
  uint displayNodes(const RenderContext& ctx);
  void displayNodeHeader(JsonPair& node, JsonObject& nodeData,
                         const RenderContext& ctx, int column, uint8_t& row,
                         uint& row_offset);
  void displayNodeMeasurements(JsonObject& nodeData, const RenderContext& ctx,
                               int column, uint8_t& row, uint& row_offset);
  void displayDeviceMeasurements(JsonObject& measurements_v2,
                                 const std::string& device,
                                 JsonObject& nodeData, int node_count,
                                 int column, uint8_t& row, uint& row_offset);
  void displayBatteryLevel(JsonObject& nodeData, int node_count, int column,
                           uint8_t& row, uint& row_offset);
  void displayBatteryLevel(JsonObject& nodeData);
  void displayBadStatuses(JsonObject& nodeData, int node_count, int column,
                          uint8_t& row, uint& row_offset);
  void displayStaleState(JsonObject& nodeData, int node_count, int column,
                         uint8_t& row, uint& row_offset);
  std::pair<bool, std::pair<float, float>> getDeviceMinMax(
      JsonObject& nodeData, const std::string& device,
      const std::string& measurement);
  void displayLocalSensorData();
  void displayNodeVersion(JsonObject& nodeData, int node_count, int column,
                          uint8_t& row, uint& row_offset);
  bool fullRender();
  bool fullRenderInternal();
  void partialRenderInternal();

 public:
  EPDView2();
  ~EPDView2() override;

  bool render(JsonDocument* doc,
              const std::map<std::string, Sensor*>& sensors) override;
  void cleanup() override;
};
