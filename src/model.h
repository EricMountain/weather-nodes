#pragma once

#include <algorithm>
#include <map>
#include <sstream>
#include <string>

#include <ArduinoJson.h>

#include "datetime.h"

class Model {
 public:
  Model();
  Model(const std::string& json_str);
  ~Model();
  bool jsonLoadOK() const { return jsonLoadOK_; }
  void setDate(const std::string& datetime_str);
  std::string getDate() const;
  void setTime(const std::string& time_str);
  std::string getTime() const;
  void setSunInfo(const std::string& sunrise, const std::string& transit,
                  const std::string& sunset);
  std::string getSunRise() const;
  std::string getSunSet() const;
  std::string getSunTransit() const;
  void setMoonInfo(const std::string& phase, const std::string& phase_letter,
                   const std::string& rise, const std::string& transit,
                   const std::string& set);
  std::string getMoonRise() const;
  std::string getMoonSet() const;
  std::string getMoonTransit() const;
  std::string getMoonPhase() const;
  char getMoonPhaseLetter() const;
  void addNodes(JsonObject nodes, DateTime& utc_timestamp);
  void addNode(JsonPair& node, DateTime& utc_timestamp);
  void setHttpPostErrorCode(int error_code) {
    http_post_error_code_ = error_code;
  }
  void setCurrentDeviceId(const std::string& device_id) {
    current_device_id_ = device_id;
  }
  void addNodeMeasurementsV2(JsonObject& raw_node_data, JsonObject& new_node);
  void addNodeMeasurementsMinMax(JsonObject& raw_node_data,
                                 JsonObject& new_node);
  void addNodeStaleState(DateTime& utc_timestamp, JsonObject& raw_node_data,
                         JsonObject& new_node);
  void addNodeStatusSection(JsonObject& raw_node_data, JsonObject& new_node,
                            const char* device_id = "");
  void addNodeBatteryLevel(JsonObject& raw_node_data, JsonObject& new_node);
  JsonObject getNodeData() const;
  std::string toJsonString() const;
  bool fromJsonString(const std::string& json_str);
  bool operator==(const Model& other) const;
  bool operator!=(const Model& other) const;
  void buildFromJson(JsonDocument* doc, DateTime utc_timestamp,
                     DateTime local_timestamp);
  void calculateSunAndMoon(DateTime local_timestamp, JsonDocument* doc);

 private:
  JsonDocument* doc_;
  bool jsonLoadOK_ = false;
  int http_post_error_code_ = 0;
  std::string current_device_id_;
  std::string time_;
  std::string get(std::string key, std::string subkey) const;
  char batteryLevelToChar(float battery_percentage);
};
