#ifndef NODE_APP_H
#define NODE_APP_H

#include <fmt/core.h>

#include <map>
#include <string>
#include <vector>

#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

#include "config.h"
#include "datetime.h"
#include "sensor.h"

// Display view system
#ifdef HAS_DISPLAY
#include "views/display_view.h"
#endif

class NodeApp {
 public:
  NodeApp(const char* ssid, const char* password)
      : ssid_(ssid),
        password_(password)
#ifdef HAS_DISPLAY
        ,
        view_(nullptr)
#endif
  {
    doc_ = nullptr;
  }

  void setup();
  void updateDisplay();
  void setJsonDoc(JsonDocument* d) { doc_ = d; }
  void goToSleep();
  void doApiCalls();

 private:
  const char* ssid_;
  const char* password_;
#ifdef HAS_DISPLAY
  DisplayView* view_;
#endif

  std::map<std::string, Sensor*> sensors_;

  JsonDocument* doc_;
  int http_post_error_code_ = 0;  // 0 means no error
  std::string device_id_;

  void setupSerial();
  void registerSensors();
  void setupWiFi();
  std::string buildPayload();
  void registerResultsBME680(
      std::vector<std::pair<std::string, std::string>>& status,
      std::vector<std::string>& device_measurements);
  void registerResultsBattery(
      std::vector<std::pair<std::string, std::string>>& status,
      std::vector<std::string>& device_measurements);
  void registerResultsSHT31D(
      std::vector<std::pair<std::string, std::string>>& status,
      std::vector<std::string>& device_measurements);
  void registerResultsWiFi(
      std::vector<std::pair<std::string, std::string>>& status,
      std::vector<std::string>& device_measurements);
  void formatMeasurementsPayload(std::vector<std::string>& device_measurements,
                                 std::string& measurements_v2);
  std::string formatStatusPayload(
      std::vector<std::pair<std::string, std::string>>& status);
  void doPost(WiFiClientSecure& client);
#ifdef HAS_DISPLAY
  void doGet(WiFiClientSecure& client);
#endif
#ifdef OTA_UPDATE_ENABLED
  void handlePostResponse(String response);
  void updateFirmware(const char* firmware_url);
#endif
};

#endif
