#pragma once

#include <ArduinoJson.h>
#include <map>
#include <string>

#include "controller.h"
#include "datetime.h"
#include "model.h"
#include "sensor.h"

/**
 * Abstract base class for display renderers.
 * Implement this interface to create different display backends
 * (EPD, LCD, OLED, etc.) that can be selected at compile time.
 */
class DisplayView {
 public:
  virtual ~DisplayView() = default;

  /**
   * Build the display model from JSON data and sensors.
   * Returns true if display should be refreshed, false otherwise.
   */
  virtual bool buildModel(JsonDocument* doc,
                          const std::map<std::string, Sensor*>& sensors);

  /**
   * Set the HTTP POST error code for display.
   */
  void setHttpPostErrorCode(int error_code) {
    http_post_error_code_ = error_code;
  }

  /**
   * Set the current device ID for POST error reporting.
   */
  void setCurrentDeviceId(const std::string& device_id) {
    current_device_id_ = device_id;
  }

  /**
   * Render the model to the display hardware.
   */
  virtual void render() = 0;

  /**
   * Partial render of the model to the display hardware.
   */
  virtual void partialRender() = 0;

  /**
   * Cleanup display resources (e.g., put display to sleep).
   */
  virtual void cleanup() = 0;

 protected:
  bool doc_is_valid_ = false;
  Model model_;
  DateTime utc_timestamp_;
  DateTime local_timestamp_;
  std::map<std::string, Sensor*> sensors_;
  int http_post_error_code_ = 0;
  std::string current_device_id_;

  /**
   * Parse a timestamp value from the JSON document.
   */
  DateTime parseTimestampValue(JsonDocument* doc, const String& timestamp_key);

  /**
   * Parse a timestamp string into a DateTime object.
   */
  DateTime parseTimestamp(const std::string& timestamp,
                          const String& timestamp_key);
};
