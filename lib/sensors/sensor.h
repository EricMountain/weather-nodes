#pragma once

#include <map>
#include <string>

// Structure to hold a measurement value and its unit
struct Measurement {
  float value;
  std::string unit;
};

// Abstract base class for sensors
class Sensor {
 public:
  virtual ~Sensor() = default;

  // Initialize the sensor (e.g., hardware setup)
  virtual bool init() = 0;

  // Report if the sensor is working correctly
  virtual bool ok() const = 0;

  // Read measurements from the sensor
  // Returns a map: measurement name -> Measurement struct
  virtual std::map<std::string, Measurement> read() = 0;
};
