#pragma once

#include <ArduinoJson.h>
#include <map>
#include <string>

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
                          const std::map<std::string, Sensor*>& sensors) = 0;

  /**
   * Render the model to the display hardware.
   */
  virtual void render() = 0;

  /**
   * Check if display needs to be refreshed based on model changes.
   */
  virtual bool needsRefresh() const = 0;

  /**
   * Cleanup display resources (e.g., put display to sleep).
   */
  virtual void cleanup() = 0;
};
