#ifndef BATTERY_H
#define BATTERY_H

#include <stdint.h>

#include <Arduino.h>

#include "config.h"
#include "sensor.h"

#ifdef HAS_BATTERY

class BatterySensor : public Sensor
{
public:
  BatterySensor() {};

  bool init() override
  {
    // No initialization needed for battery monitoring
    Serial.printf("Battery voltage: %.2f V (raw %d)\n", getBatteryVoltage(), getBatteryVoltageRaw());
    return true;
  }

  bool ok() const override
  {
    // Always return true as long as the pin is readable
    return true;
  }

  std::map<std::string, Measurement> read() override
  {
    std::map<std::string, Measurement> data;
    float voltage = getBatteryVoltage();
    float percentage = getBatteryPercentage();
    data["battery_voltage"] = {voltage, "V"};
    data["battery_percentage"] = {percentage, "%"};
    return data;
  }

private:
  // Source: https://github.com/ThingPulse/esp32-epulse-feather-testbed/blob/main/src/main.cpp
  uint16_t getBatteryVoltageRaw()
  {
    return analogRead(BAT_MON_PIN);
  }

  float getBatteryVoltage()
  {
    // VOut * (R1 + R2)/R2
    float rawVoltage = getBatteryVoltageRaw() * 3.3 / 4095.0;
    float batteryVoltage = rawVoltage * (R1 + R2) / R2;
    return batteryVoltage;
  }

  float getBatteryPercentage()
  {
    float voltage = getBatteryVoltage();
    if (voltage >= 4.2)
      return 100.0;
    if (voltage <= 3.3)
      return 0.0;
    return (voltage - 3.3) / (4.2 - 3.3) * 100.0;
  }
};

#endif

#endif
