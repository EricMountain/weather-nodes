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

  ~NodeApp() {
    Serial.println("Cleaning up NodeApp...");
    for (auto& sensor : sensors_) {
      delete sensor.second;
    }
    sensors_.clear();

    if (doc_ != nullptr) {
      delete doc_;
    }

#ifdef HAS_DISPLAY
    if (view_ != nullptr) {
      view_->cleanup();
      delete view_;
    }
#endif
  }

  bool setup();
  bool updateDisplay();
  void setJsonDoc(JsonDocument* d) { doc_ = d; }
  void doApiCalls();

 private:
  const char* ssid_;
  const char* password_;
  WiFiClientSecure client_;
#ifdef HAS_DISPLAY
  DisplayView* view_;
#endif

  std::map<std::string, Sensor*> sensors_;

  JsonDocument* doc_;
  int http_post_error_code_ = 0;
  std::string device_id_;

  void registerSensors();
  bool setupWiFi();
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
  void registerResultsFreeHeap(
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
