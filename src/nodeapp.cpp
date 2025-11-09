#include "nodeapp.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#ifdef OTA_UPDATE_ENABLED
#include <Update.h>
#endif

#include "certs.h"
#include "config.h"
#include "secrets.h"
#include "wifi_quality.h"

#ifdef HAS_DISPLAY
#include <LittleFS.h>

#include "epd_view_2.h"
#endif

#if defined(HAS_BME680) || defined(HAS_SHT31D)
#include <Adafruit_Sensor.h>
#include <Wire.h>

#ifdef HAS_BME680
#include "bme680.h"
#endif

#ifdef HAS_SHT31D
#include "sht31d.h"
#endif
#endif

#ifdef HAS_BATTERY
#include "battery.h"
#endif

#include "version.h"

bool NodeApp::setup() {
  if (!setupWiFi()) {
    return false;
  }
  registerSensors();
#ifdef HAS_DISPLAY
  if (view_ == nullptr) {
    view_ = new EPDView2();
  } else {
    Serial.println("Display view already initialized");
  }
#endif
  Serial.printf("Weather Node git commit: %s\n", GIT_COMMIT_HASH);
  return true;
}

bool NodeApp::setupWiFi() {
  int attempts = 20;
  Serial.print("Connecting to WiFi");
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(this->ssid_, this->password_);
  }

  while (WiFi.status() != WL_CONNECTED) {
    delay(500 / (attempts < 1 ? 1 : attempts));
    Serial.print(".");
    if (--attempts <= 0) {
      Serial.println(
          "\nFailed to connect to WiFi, retrying later, going to sleep...");
      return false;
    }
  }
  Serial.printf("\nWiFi connected, link quality: %d dBm\n", WiFi.RSSI());
  Serial.printf("Local IP: %s\n", WiFi.localIP().toString().c_str());
  return true;
}

void NodeApp::registerSensors() {
#ifdef HAS_BME680
  sensors_["bme680"] = new BME680Sensor();
  if (!(sensors_["bme680"])->init()) {
    Serial.println("Failed to initialize BME680 sensor");
  }
#endif

#ifdef HAS_SHT31D
  sensors_["sht31d"] = new SHT31DSensor();
  if (!sensors_["sht31d"]->init()) {
    Serial.println("Failed to initialize SHT31D sensor");
  }
#endif

#ifdef HAS_BATTERY
  sensors_["battery"] = new BatterySensor();
  if (!sensors_["battery"]->init()) {
    Serial.println("Failed to initialize Battery sensor");
  }
#endif

  sensors_["wifi"] = new WiFiSensor();
  if (!sensors_["wifi"]->init()) {
    Serial.println("Failed to initialize WiFi sensor");
  }
}

void NodeApp::doApiCalls() {
  client_.setCACert(rootCACerts);
  doPost(client_);
#ifdef HAS_DISPLAY
  doGet(client_);
#endif
  // client_.stop();
}

void NodeApp::doPost(WiFiClientSecure& client) {
  HTTPClient httpPost;
  httpPost.addHeader("x-api-key", API_KEY);
  Serial.println("[HTTPS] begin...");
  if (httpPost.begin(client, POST_URL)) {
    Serial.println("[HTTPS] POST...");
    std::string payload = buildPayload();
    // TODO: implement retries with exponential backoff
    int httpCode = httpPost.POST(String(payload.c_str()));
    if (httpCode > 0) {
      Serial.printf("[HTTPS] POST... code: %d\n", httpCode);
      String payload = httpPost.getString();
      Serial.println(payload);
#ifdef OTA_UPDATE_ENABLED
      handlePostResponse(payload);
#endif
    } else {
      Serial.printf("[HTTPS] POST... failed, error: %s\n",
                    httpPost.errorToString(httpCode).c_str());
    }
    http_post_error_code_ = httpCode;
  }
  httpPost.end();
}

std::string NodeApp::buildPayload() {
  std::vector<std::pair<std::string, std::string>> status;
  std::vector<std::string> device_measurements;

  registerResultsWiFi(status, device_measurements);
  registerResultsBME680(status, device_measurements);
  registerResultsBattery(status, device_measurements);
  registerResultsSHT31D(status, device_measurements);
  registerResultsFreeHeap(status, device_measurements);

  std::string measurements_v2;
  formatMeasurementsPayload(device_measurements, measurements_v2);

  std::string status_str = formatStatusPayload(status);

  std::string version = fmt::format(R"("version": "{}")", GIT_COMMIT_HASH);

  std::string payload =
      fmt::format(R"({{{}, {}, {}}})", measurements_v2, status_str, version);

  Serial.printf("POST data: %s\n", payload.c_str());
  return payload;
}

void NodeApp::registerResultsSHT31D(
    std::vector<std::pair<std::string, std::string>>& status,
    std::vector<std::string>& device_measurements) {
#ifdef HAS_SHT31D
  if (sensors_.find("sht31d") == sensors_.end() || !sensors_["sht31d"]->ok()) {
    status.push_back(std::pair<std::string, std::string>{"sht31d", "error"});
  } else {
    status.push_back(std::pair<std::string, std::string>{"sht31d", "ok"});
    std::map<std::string, Measurement> measurements =
        sensors_["sht31d"]->read();
    std::string sht31d_fmt = fmt::format(
        R"("sht31d": {{"temperature": {:.2f}, "humidity": {:.2f}}})",
        measurements["temperature"].value, measurements["humidity"].value);
    device_measurements.push_back(sht31d_fmt);
  }
#endif
}

void NodeApp::registerResultsBME680(
    std::vector<std::pair<std::string, std::string>>& status,
    std::vector<std::string>& device_measurements) {
#ifdef HAS_BME680
  if (sensors_.find("bme680") == sensors_.end() || !sensors_["bme680"]->ok()) {
    status.push_back(std::pair<std::string, std::string>{"bme680", "error"});
  } else {
    status.push_back(std::pair<std::string, std::string>{"bme680", "ok"});
    std::map<std::string, Measurement> measurements =
        sensors_["bme680"]->read();
    std::string bme_fmt = fmt::format(
        R"("bme680": {{"temperature": {:.2f}, "humidity": {:.2f}, "pressure": {:.0f}}})",
        measurements["temperature"].value, measurements["humidity"].value,
        measurements["pressure"].value);
    device_measurements.push_back(bme_fmt);
  }
#endif
}

void NodeApp::registerResultsBattery(
    std::vector<std::pair<std::string, std::string>>& status,
    std::vector<std::string>& device_measurements) {
#ifdef HAS_BATTERY
  if (sensors_.find("battery") == sensors_.end() ||
      !sensors_["battery"]->ok()) {
    status.push_back(std::pair<std::string, std::string>{"battery", "error"});
  } else {
    status.push_back(std::pair<std::string, std::string>{"battery", "ok"});
    std::map<std::string, Measurement> measurements =
        sensors_["battery"]->read();
    std::string battery_fmt = fmt::format(
        R"("battery": {{"battery_voltage": {:.2f}, "battery_percentage": {:.0f}}})",
        measurements["voltage"].value, measurements["percent"].value);
    device_measurements.push_back(battery_fmt);
  }
#endif
}

void NodeApp::registerResultsWiFi(
    std::vector<std::pair<std::string, std::string>>& status,
    std::vector<std::string>& device_measurements) {
  if (sensors_.find("wifi") == sensors_.end() || !sensors_["wifi"]->ok()) {
    status.push_back(std::pair<std::string, std::string>{"wifi", "error"});
  } else {
    status.push_back(std::pair<std::string, std::string>{"wifi", "ok"});
    std::map<std::string, Measurement> measurements = sensors_["wifi"]->read();
    std::string wifi_fmt = fmt::format(R"("wifi": {{"wifi_dbm": {:.0f}}})",
                                       measurements["wifi_dbm"].value);
    device_measurements.push_back(wifi_fmt);
  }
}

void NodeApp::registerResultsFreeHeap(
    std::vector<std::pair<std::string, std::string>>& status,
    std::vector<std::string>& device_measurements) {
  size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  std::string heap_fmt =
      fmt::format(R"("system": {{"free_heap_bytes": {}}})", free_heap);
  device_measurements.push_back(heap_fmt);
}

void NodeApp::formatMeasurementsPayload(
    std::vector<std::string>& device_measurements,
    std::string& measurements_v2) {
  measurements_v2 = fmt::format(R"("measurements_v2": {{)");
  bool first = true;
  for (const auto& measurement : device_measurements) {
    if (first) {
      first = false;
    } else {
      measurements_v2 += ", ";
    }
    measurements_v2 += fmt::format(R"({})", measurement);
  }
  measurements_v2 += "}";
  Serial.println(measurements_v2.c_str());
}

std::string NodeApp::formatStatusPayload(
    std::vector<std::pair<std::string, std::string>>& status) {
  std::string status_str = "\"status\": {}";
  if (!status.empty()) {
    status_str = fmt::format(R"("status": {{)");
    bool first = true;
    for (const auto& s : status) {
      if (first) {
        first = false;
      } else {
        status_str += ", ";
      }
      status_str += fmt::format(R"("{}": "{}")", s.first, s.second);
    }
    status_str += "}";
  }

  return status_str;
}

#ifdef HAS_DISPLAY
void NodeApp::doGet(WiFiClientSecure& client) {
  JsonDocument* doc = nullptr;
  int attempts = 3;

  while (attempts-- > 0) {
    HTTPClient httpGet;
    httpGet.begin(client, GET_URL);
    httpGet.addHeader("x-api-key", API_KEY);
    int httpCode = httpGet.GET();
    if (httpCode > 0) {
      Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
      String payload = httpGet.getString();
      Serial.println(payload);

      doc = new JsonDocument();
      DeserializationError error = deserializeJson(*doc, payload);
      if (error) {
        Serial.print(F("JSON parse failed: "));
        Serial.println(error.f_str());
        delete doc;
        doc = nullptr;
      }
    } else {
      Serial.printf("[HTTPS] GET... failed, error: %s\n",
                    httpGet.errorToString(httpCode).c_str());
    }
    httpGet.end();

    if (doc != nullptr) {
      break;
    }
    delay(1000 * (4 - attempts));  // Wait longer for each retry
  }

  if (doc != nullptr) {
    // Extract device_id from the top level of the response
    if ((*doc)["device_id"].is<JsonString>()) {
      device_id_ = (*doc)["device_id"].as<std::string>();
      Serial.printf("Device ID from response: %s\n", device_id_.c_str());
    }
  }

  // TODO: check if there is a better place to do this, e.g. after model is
  // built
  if (doc_ != nullptr) {
    delete doc_;
  }
  doc_ = doc;
}

// Returns true if deep sleep is needed
bool NodeApp::updateDisplay() {
  if (view_ == nullptr) {
    Serial.println("View not initialized");
    return true;
  }
  view_->setHttpPostErrorCode(http_post_error_code_);
  view_->setCurrentDeviceId(device_id_);
  bool deep_sleep_needed = view_->render(doc_, sensors_);
  return deep_sleep_needed;
}
#endif

#ifdef OTA_UPDATE_ENABLED
void NodeApp::handlePostResponse(String response) {
  JsonDocument doc = JsonDocument();
  DeserializationError error = deserializeJson(doc, response);
  if (error) {
    Serial.print(F("JSON parse failed: "));
    Serial.println(error.f_str());
    return;
  }

  if (doc["ota_update"].is<JsonObject>()) {
    JsonObject ota_update = doc["ota_update"].as<JsonObject>();
    String url = ota_update["url"] | "";
    if (url.length() > 0) {
      updateFirmware(url.c_str());
    }
  }
}

void NodeApp::updateFirmware(const char* firmware_url) {
  WiFiClientSecure client;
  client.setCACert(rootCACerts);

  HTTPClient https;
  Serial.printf("Starting OTA from: %s\n", firmware_url);

  if (https.begin(client, firmware_url)) {
    int httpCode = https.GET();
    if (httpCode == HTTP_CODE_OK) {
      int contentLength = https.getSize();
      bool canBegin = Update.begin(contentLength);
      if (canBegin) {
        Serial.printf("Starting download. OTA size: %u bytes\n", contentLength);
        WiFiClient* stream = https.getStreamPtr();
        size_t written = Update.writeStream(*stream);
        if (written == contentLength) {
          Serial.println(F("OTA written successfully. Rebooting..."));
          if (Update.end()) {
            if (Update.isFinished()) {
              Serial.println(F("Update successfully completed. Rebooting."));
              ESP.restart();
            } else {
              Serial.println(F("Update not finished? Something went wrong!"));
            }
          } else {
            Serial.printf("Update.end() error: %s\n", Update.errorString());
          }
        } else {
          Serial.printf("OTA written only %u/%u bytes. Aborting.\n", written,
                        contentLength);
        }
      } else {
        Serial.println(F("Not enough space to begin OTA"));
      }
    } else {
      Serial.print(F("OTA HTTPS GET failed, error: "));
      Serial.printf("%d %s\n", httpCode, https.errorToString(httpCode).c_str());
    }
    https.end();
  } else {
    Serial.println(F("Unable to connect to OTA server"));
  }
}
#endif
