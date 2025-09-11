#include "nodeapp.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#include "config.h"
#include "secrets.h"
#include "wifi_quality.h"
#include "certs.h"

#ifdef HAS_DISPLAY
#include "sunandmoon.h"
#include "fonts/moon_phases_48pt.h"
#endif

#if defined(HAS_BME680) || defined(HAS_SHT31D)
#include <Wire.h>
#include <Adafruit_Sensor.h>

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

void NodeApp::setup()
{
  setupSerial();
  setupWiFi();
  registerSensors();

#ifdef HAS_DISPLAY
  display_ = new GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>(GxEPD2_750_T7(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));
  Serial.println("Display initialized");
#endif
}

void NodeApp::setupSerial()
{
  int attempts = 20;
  Serial.begin(115200);
  while (!Serial)
  {
    delay(10); // Wait for serial monitor connection
    if (--attempts <= 0)
    {
      Serial.println("Gave up waiting for serial monitor, continuing... (so this shouldn't appear...)");
      break;
    }
  }
  Serial.println("Booting...");
}

void NodeApp::setupWiFi()
{
  int attempts = 20;
  Serial.print("Connecting to WiFi");
  WiFi.begin(this->ssid_, this->password_);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    if (--attempts <= 0)
    {
      Serial.println("Failed to connect to WiFi, retrying later, going to sleep...");
      goToSleep();
    }
  }
  Serial.printf("\nWiFi connected, link quality: %d dBm\n", WiFi.RSSI());
  Serial.printf("Local IP: %s\n", WiFi.localIP().toString().c_str());
}

void NodeApp::goToSleep()
{
  Serial.printf("Entering deep sleep for %d seconds...\n", SLEEP_SECONDS);
  esp_sleep_enable_timer_wakeup(SLEEP_SECONDS * 1000000ULL); // microseconds
#ifdef HAS_DISPLAY
  this->display_->hibernate(); // Save even more power (if supported by display)
#endif
#ifndef HAS_BATTERY
  delay(100); // Let serial print
#endif
  esp_deep_sleep_start(); // Go to sleep
}

void NodeApp::registerSensors()
{
#ifdef HAS_BME680
  sensors_["bme680"] = new BME680Sensor();
  if (!(this->sensors_["bme680"])->init())
  {
    Serial.println("Failed to initialize BME680 sensor");
  }
#endif

#ifdef HAS_SHT31D
  sensors_["sht31d"] = new SHT31DSensor();
  if (!sensors_["sht31d"]->init())
  {
    Serial.println("Failed to initialize SHT31D sensor");
  }
#endif

#ifdef HAS_BATTERY
  sensors_["battery"] = new BatterySensor();
  if (!sensors_["battery"]->init())
  {
    Serial.println("Failed to initialize Battery sensor");
  }
#endif

  sensors_["wifi"] = new WiFiSensor();
  if (!sensors_["wifi"]->init())
  {
    Serial.println("Failed to initialize WiFi sensor");
  }
}

void NodeApp::doApiCalls()
{
  WiFiClientSecure client;
  JsonDocument *doc = nullptr;

  client.setCACert(rootCACerts);
  doPost(client);
#ifdef HAS_DISPLAY
  doGet(client);
#endif
  client.stop();
}

void NodeApp::doPost(WiFiClientSecure &client)
{
  Serial.println("Doing POST...");
#ifdef HAS_DISPLAY
  if (!sensors_["bme680"]->ok())
  {
    Serial.println("Skipping POST because BME680 is not available");
    return;
  }
  else
  {
    Serial.println("BME680 is available");
  }
#endif
  HTTPClient httpPost;
  httpPost.addHeader("x-api-key", API_KEY);
  Serial.println("[HTTPS] begin...");
  if (httpPost.begin(client, POST_URL))
  {
    Serial.println("[HTTPS] POST...");
    std::string payload = buildPayload();
    int httpCode = httpPost.POST(String(payload.c_str()));
    // httpCode is negative on error
    if (httpCode > 0)
    {
      Serial.printf("[HTTPS] POST... code: %d\n", httpCode);
      String payload = httpPost.getString();
      Serial.println(payload);
    }
    else
    {
      Serial.printf("[HTTPS] POST... failed, error: %s\n", httpPost.errorToString(httpCode).c_str());
      // TODO: flag error for later display
    }
  }
  httpPost.end();
}

std::string NodeApp::buildPayload()
{
  std::vector<std::pair<std::string, std::string>> status;
  std::vector<std::string> device_measurements;

  std::map<std::string, Measurement> wifi_measurements = sensors_["wifi"]->read();
  std::string wifi_fmt = fmt::format(R"("wifi": {{"wifi_dbm": {:.0f}}})", wifi_measurements["wifi_dbm"].value);
  device_measurements.push_back(wifi_fmt);

#ifdef HAS_BME680
  if (sensors_.find("bme680") == sensors_.end() || !sensors_["bme680"]->ok())
  {
    status.push_back(std::pair<std::string, std::string>{"bme680", "error"});
  }
  else
  {
    status.push_back(std::pair<std::string, std::string>{"bme680", "ok"});
    std::map<std::string, Measurement> measurements = sensors_["bme680"]->read();
    std::string bme_fmt = fmt::format(R"("bme680": {{"temperature": {:.2f}, "humidity": {:.2f}, "pressure": {:.0f}}})",
                                      measurements["temperature"].value,
                                      measurements["humidity"].value,
                                      measurements["pressure"].value);
    device_measurements.push_back(bme_fmt);
  }
#endif
#ifdef HAS_BATTERY
  std::map<std::string, Measurement> battery_measurements = sensors_["battery"]->read();
  std::string battery_fmt = fmt::format(R"("battery": {{"battery_voltage": {:.2f}, "battery_percentage": {:.2f}}})",
                                        battery_measurements["battery_voltage"].value,
                                        battery_measurements["battery_percentage"].value);
  device_measurements.push_back(battery_fmt);
#endif
#ifdef HAS_SHT31D
  if (sensors_.find("sht31d") == sensors_.end() || !sensors_["sht31d"]->ok())
  {
    status.push_back(std::pair<std::string, std::string>{"sht31d", "error"});
  }
  else
  {
    status.push_back(std::pair<std::string, std::string>{"sht31d", "ok"});
    std::map<std::string, Measurement> measurements = sensors_["sht31d"]->read();
    std::string sht31d_fmt = fmt::format(R"("sht31d": {{"temperature": {:.2f}, "humidity": {:.2f}}})",
                                         measurements["temperature"].value,
                                         measurements["humidity"].value);
    device_measurements.push_back(sht31d_fmt);
  }
#endif

  std::string measurements_v2;
  formatMeasurementsPayload(device_measurements, measurements_v2);

  std::string status_str = formatStatusPayload(status);
  std::string payload = fmt::format(R"({{{}, {}}})",
                                    measurements_v2,
                                    status_str);

  Serial.printf("POST data: %s\n", payload.c_str());
  return payload;
}

void NodeApp::formatMeasurementsPayload(std::vector<std::__cxx11::string> &device_measurements, std::__cxx11::string &measurements_v2)
{
  measurements_v2 = fmt::format(R"("measurements_v2": {{)");
  bool first = true;
  for (const auto &measurement : device_measurements)
  {
    if (first)
    {
      first = false;
    }
    else
    {
      measurements_v2 += ", ";
    }
    measurements_v2 += fmt::format(R"({})", measurement);
  }
  measurements_v2 += "}";
  Serial.println(measurements_v2.c_str());
}

std::string NodeApp::formatStatusPayload(std::vector<std::pair<std::__cxx11::string, std::__cxx11::string>> &status)
{
  std::string status_str = "\"status\": {}";
  if (!status.empty())
  {
    status_str = fmt::format(R"("status": {{)");
    bool first = true;
    for (const auto &s : status)
    {
      if (first)
      {
        first = false;
      }
      else
      {
        status_str += ", ";
      }
      status_str += fmt::format(R"("{}": "{}")", s.first, s.second);
    }
    status_str += "}";
  }

  return status_str;
}

#ifdef HAS_DISPLAY
void NodeApp::doGet(WiFiClientSecure &client)
{
  JsonDocument *doc = nullptr;

  HTTPClient httpGet;
  httpGet.begin(client, GET_URL);
  httpGet.addHeader("x-api-key", API_KEY);
  int httpCode = httpGet.GET();
  if (httpCode > 0)
  {
    Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
    String payload = httpGet.getString();
    Serial.println(payload);

    doc = new JsonDocument();
    DeserializationError error = deserializeJson(*doc, payload);
    if (error)
    {
      Serial.print(F("JSON parse failed: "));
      Serial.println(error.f_str());
      delete doc;
      doc = nullptr;
    }
    else
    {
      Serial.println(F("JSON parsed successfully"));
    }
  }
  else
  {
    Serial.printf("[HTTPS] GET... failed, error: %s\n", httpGet.errorToString(httpCode).c_str());
  }
  httpGet.end();

  doc_ = doc;
}
#endif

#ifdef HAS_DISPLAY
void NodeApp::moonPhase(char phase)
{
  u8g2_.setFont(moon_phases_48pt);
  u8g2_.print(phase);
  u8g2_.setFont(defaultFont);
}

void NodeApp::updateDisplay()
{
  JsonDocument *doc = this->doc_;
  GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> &display = *this->display_;
  
  display.init(115200);
  Serial.println("E-Paper display initialized");

  display.setRotation(0); // Landscape

  u8g2_.begin(display);

  display.setFullWindow();
  display.firstPage();
  do
  {
    u8g2_.setFontMode(0);
    u8g2_.setFontDirection(0);
    u8g2_.setForegroundColor(GxEPD_BLACK);
    u8g2_.setBackgroundColor(GxEPD_WHITE);
    u8g2_.setFont(defaultFont);

    u8g2_.setCursor(0, 50);

    // TODO: need to distinguish GET failure vs partial response
    if (doc == nullptr || doc->isNull() || !doc->operator[]("nodes").is<JsonObject>())
    {
      u8g2_.println("Failed to get data - local sensor only");
      if (!sensors_["bme680"]->ok())
      {
        u8g2_.println("Local sensor (BME680) setup failed");
      }
      else
      {
        std::map<std::string, Measurement> measurements = sensors_["bme680"]->read();
        for (const auto &measurement : measurements)
        {
          u8g2_.printf("%s: ", measurement.first.c_str());
          Measurement m = measurement.second;
          u8g2_.printf("%.2f %s\n", m.value, m.unit.c_str());
        }
      }
    }
    else
    {
      // Get UTC timestamp to check for stale data
      DateTime utc_timestamp = parseTimestamp("timestamp_utc");
      DateTime local_timestamp = parseTimestamp("timestamp_local");

      if (local_timestamp.ok())
      {
        Serial.printf("Local time: %s\n", local_timestamp.format("%A %d %B %Y").c_str());
        u8g2_.printf("%s  ", local_timestamp.niceDate().c_str());

        u8g2_.println();
        u8g2_.println();

        // Get location data from response config section if available
        double latitude = 48.866667;
        double longitude = 2.333333;
        int utc_offset_seconds = 0;
        if (doc->operator[]("config").is<JsonObject>()) {
          JsonObject config = doc->operator[]("config").as<JsonObject>();
          if (config["location"].is<JsonObject>()) {
            JsonObject location = config["location"].as<JsonObject>();
            if (location["latitude"].is<JsonString>()) {
              latitude = location["latitude"].as<String>().toDouble();
            }
            if (location["longitude"].is<JsonString>()) {
              longitude = location["longitude"].as<String>().toDouble();
            }
            if (location["utc_offset_seconds"].is<JsonInteger>()) {
              utc_offset_seconds = location["utc_offset_seconds"].as<int>();
            }
          }
        }
        Serial.printf("Location: lat %.6f lon %.6f UTC offset %d seconds\n", latitude, longitude, utc_offset_seconds);
        
        SunAndMoon sunAndMoon(local_timestamp.year(), local_timestamp.month(), local_timestamp.day(),
                              local_timestamp.hour(), local_timestamp.minute(), local_timestamp.second(),
                              latitude, longitude, utc_offset_seconds);
        u8g2_.printf("Sun:  %s  %s  %s\n", sunAndMoon.getSunrise().c_str(), sunAndMoon.getSunTransit().c_str(), sunAndMoon.getSunset().c_str());
        u8g2_.printf("Moon: %s  %s  %s  ", sunAndMoon.getMoonRise().c_str(), sunAndMoon.getMoonTransit().c_str(), sunAndMoon.getMoonSet().c_str());
        moonPhase(sunAndMoon.getMoonPhaseLetter());
        u8g2_.println();
        u8g2_.printf("      %s\n", sunAndMoon.getMoonPhase().c_str());
      }
      else
      {
        u8g2_.println("(Date unknown)");
      }
      u8g2_.println();

      JsonObject nodes = doc->operator[]("nodes");

      for (JsonPair node : nodes)
      {
        JsonObject nodeData = node.value().as<JsonObject>();
        displayNodeHeader(node, nodeData, utc_timestamp);
        displayNodeMeasurements(nodeData);
        u8g2_.println();
      }
    }
  } while (display.nextPage());
}

void NodeApp::displayNodeMeasurements(JsonObject &nodeData)
{
  if (nodeData["measurements_v2"].is<JsonObject>())
  {
    JsonObject measurements_v2 = nodeData["measurements_v2"].as<JsonObject>();
    std::vector<std::string> devices = {"bme680", "sht31d"};
    for (const auto &device : devices)
    {
      displayDeviceMeasurements(measurements_v2, device, nodeData);
    }
  }
}

void NodeApp::displayDeviceMeasurements(JsonObject &measurements_v2, const std::string &device, JsonObject &nodeData)
{
  if (measurements_v2[device].is<JsonObject>())
  {
    JsonObject device_map = measurements_v2[device].as<JsonObject>();
    if (device_map["temperature"].is<JsonString>())
    {
      auto min_max = getDeviceMinMax(nodeData, device, "temperature");
      if (min_max.first)
      {
        u8g2_.printf(" %.1f(%.1f/%.1f)°C ", float(device_map["temperature"]), min_max.second.first, min_max.second.second);
      }
      else
      {
        u8g2_.printf(" %.1f°C", float(device_map["temperature"]));
      }
    }
    if (device_map["humidity"].is<JsonString>())
    {
        u8g2_.printf(" %.1f%% ", float(device_map["humidity"]));
    }
    if (device_map["pressure"].is<JsonString>())
    {
      u8g2_.printf(" %.0fhPa ", float(device_map["pressure"]));
    }
    u8g2_.println();
  }
}

std::pair<bool, std::pair<float, float>> NodeApp::getDeviceMinMax(JsonObject &nodeData, const std::string &device, const std::string &measurement)
{
  float min = -1;
  float max = -1;
  bool found = false;

  if (nodeData["measurements_min_max"].is<JsonObject>())
  {
    JsonObject min_max = nodeData["measurements_min_max"].as<JsonObject>();
    if (min_max[device].is<JsonObject>())
    {
      JsonObject device_min_max = min_max[device].as<JsonObject>();
      if (device_min_max[measurement].is<JsonObject>())
      {
        JsonObject measurement_min_max = device_min_max[measurement].as<JsonObject>();
        min = float(measurement_min_max["min"]);
        max = float(measurement_min_max["max"]);
      }
      found = true;
    }
  }

  return std::make_pair(found, std::make_pair(min, max));
}

void NodeApp::displayNodeHeader(JsonPair &node, JsonObject &nodeData, DateTime &utc_timestamp)
{
  String displayName = node.key().c_str();
  if (nodeData["display_name"].is<JsonString>())
    displayName = nodeData["display_name"].as<String>();
  u8g2_.printf("%s", displayName.c_str());

  if (nodeData["measurements_v2"].is<JsonObject>())
  {
    JsonObject measurements_v2 = nodeData["measurements_v2"].as<JsonObject>();
    if (measurements_v2["battery"].is<JsonObject>())
    {
      JsonObject battery = measurements_v2["battery"].as<JsonObject>();
      if (battery["battery_percentage"].is<JsonString>())
      {
        float battery_percentage = float(battery["battery_percentage"]);
        printBatteryLevel(battery_percentage);
      }
    }
  }

  displayBadStatuses(nodeData);

  if (utc_timestamp.ok())
  {
    if (nodeData["timestamp_utc"].is<JsonString>())
    {
      String measurements_timestamp_utc = nodeData["timestamp_utc"].as<String>();
      struct tm tm_node_utc;
      DateTime node_utc_dt = parseTimestampString(measurements_timestamp_utc, displayName + "/timestamp_utc");
      if (node_utc_dt.ok())
      {
        double diff = utc_timestamp.diff(node_utc_dt);
        if (diff < 0)
        {
          u8g2_.printf(" Time travel %.0f\"!", -diff);
        }
        else if (diff > MAX_STALE_SECONDS)
        {
          u8g2_.printf(" %.0f' old", diff / 60);
        }
      }
      else
      {
        u8g2_.printf(" (%s)", measurements_timestamp_utc.c_str());
        Serial.printf("Bad timestamp: %s\n", measurements_timestamp_utc.c_str());
      }
    }
  }
  else
  {
    u8g2_.printf(" (No reference time)");
  }
  u8g2_.println();
}

void NodeApp::displayBadStatuses(ArduinoJson::V742PB22::JsonObject &nodeData)
{
  // Display status entries that are not "ok"
  if (nodeData["status"].is<JsonObject>())
  {
    JsonObject status = nodeData["status"].as<JsonObject>();
    for (JsonPair kvp : status)
    {
      String value = kvp.value().as<String>();
      if (value != "ok")
      {
        String key = kvp.key().c_str();
        u8g2_.printf(" %s=%s", key.c_str(), value.c_str());
      }
    }
  }
}

DateTime NodeApp::parseTimestamp(const String &timestamp_key)
{
  JsonDocument *doc = this->doc_;
  DateTime dt;

  if (doc->operator[](timestamp_key).is<JsonString>())
  {
    String timestamp = doc->operator[](timestamp_key).as<String>();

    dt = parseTimestampString(timestamp, timestamp_key);
  }
  return dt;
}

DateTime NodeApp::parseTimestampString(const String &timestamp, const String &timestamp_key)
{
  DateTime dt(timestamp);
  if (!dt.ok())
  {
    Serial.printf("Failed to parse %s: %s\n", timestamp_key.c_str(), timestamp.c_str());
  }
  return dt;
}

void NodeApp::printBatteryLevel(float battery_percentage)
{
  // List of characters for battery indicator, from empty to full
  char battery_chars[] = {'0', '5', '6', '7', '8', '9', ':', ';', '<'};
  int num_levels = sizeof(battery_chars) / sizeof(battery_chars[0]);
  float level = battery_percentage / 100.0 * (num_levels - 1);

  if (level < 0)
    level = 0;
  if (level >= num_levels)
    level = num_levels - 1;
  int char_offset = round(level);
  Serial.printf("Battery percentage: %.1f%%, level: %.1f, char_offset: %d\n", battery_percentage, level, char_offset);

  u8g2_.print(" ");
  u8g2_.setFont(u8g2_font_battery24_tr);
  u8g2_.print(battery_chars[char_offset]);
  u8g2_.setFont(defaultFont);
}
#endif
