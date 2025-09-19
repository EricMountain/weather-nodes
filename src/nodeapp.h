#ifndef NODE_APP_H
#define NODE_APP_H

#include <vector>
#include <map>
#include <string>
#include <fmt/core.h>

// TODO: eliminate this dependency for outdoor nodes, or actually use it
// #ifdef HAS_DISPLAY
#include <ArduinoJson.h>
// #endif

#include <WiFiClientSecure.h>

#include "config.h"
#include "sensor.h"
#include "datetime.h"

// Screen
#ifdef HAS_DISPLAY
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "fonts/u8g2_font_battery24_tr.h"

#include "model.h"

// Pin mapping for many ESP32 dev boards and Waveshare 7.5in V2 SPI displays:
// - from perplexity
#define EPD_CS 5
#define EPD_DC 17
#define EPD_RST 16
#define EPD_BUSY 4

// - from https://www.waveshare.com/wiki/E-Paper_ESP32_Driver_Board#Pins
// #define EPD_CS   15
// #define EPD_DC   27
// #define EPD_RST  26
// #define EPD_BUSY 25

// #define SCK_pin  13 // 18
// #define MISO_pin -1
// #define MOSI_pin 14 // 23
#endif


class NodeApp
{
public:
  NodeApp(const char *ssid, const char *password)
      : ssid_(ssid), password_(password)
#ifdef HAS_DISPLAY
        ,
        display_(nullptr)
#endif
  {
    doc_ = nullptr;
#ifdef HAS_DISPLAY
    model_ = Model();
#endif
    bmeOK_ = false;
  }

  void setup();
  void updateDisplay();
  void setJsonDoc(JsonDocument *d) { doc_ = d; }
  void setBmeOK(bool ok) { bmeOK_ = ok; }
  void goToSleep();
  void doApiCalls();

private:
  const char *ssid_;
  const char *password_;
#ifdef HAS_DISPLAY
  GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> *display_;
  U8G2_FOR_ADAFRUIT_GFX u8g2_;
  const uint8_t *defaultFont = u8g2_font_inb24_mf;
  Model model_;
  DateTime utc_timestamp_;
  DateTime local_timestamp_;
#endif

  std::map<std::string, Sensor*> sensors_;

  JsonDocument *doc_;

  // TODO remove
  bool bmeOK_;

  void setupSerial();
  void registerSensors();
  void setupWiFi();
  void moonPhase();
  DateTime parseTimestamp(const String &timestamp_key);
  DateTime parseTimestampString(const std::string &timestamp, const String &timestamp_key);
  std::string buildPayload();
  void registerResultsBME680(std::vector<std::pair<std::string, std::string>> &status, std::vector<std::string> &device_measurements);
  void registerResultsBattery(std::vector<std::pair<std::string, std::string>> &status, std::vector<std::string> &device_measurements);
  void registerResultsSHT31D(std::vector<std::pair<std::string, std::string>> &status, std::vector<std::string> &device_measurements);
  void formatMeasurementsPayload(std::vector<std::string> &device_measurements, std::string &measurements_v2);
  std::string formatStatusPayload(std::vector<std::pair<std::string, std::string>> &status);
  void doPost(WiFiClientSecure &client);
#ifdef HAS_DISPLAY
  void doGet(WiFiClientSecure &client);
  bool buildDisplayModel();
  void displayNodeHeader(JsonPair &node, JsonObject &nodeData, DateTime &utc_timestamp);
  void displayBadStatuses(JsonObject &nodeData);
  void displayNodeMeasurements(JsonObject &nodeData);
  void displayDeviceMeasurements(JsonObject &measurements_v2, const std::string &device, JsonObject &nodeData);
  std::pair<bool, std::pair<float, float>>getDeviceMinMax(JsonObject &nodeData, const std::string &device, const std::string &measurement);
  void printBatteryLevel(JsonString battery_level);
#endif
};



#endif
