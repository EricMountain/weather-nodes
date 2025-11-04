#ifndef WIFI_QUALITY_H
#define WIFI_QUALITY_H

#include <stdint.h>

class WiFiSensor : public Sensor {
 public:
  WiFiSensor() {};

  bool init() override {
    // No initialization needed for WiFi monitoring
    int8_t dBm = WiFi.RSSI();
    Serial.printf("WiFi sensor init: %ddBm %d%%\n", dBm, getWifiQuality(dBm));
    return true;
  }

  bool ok() const override { return true; }

  std::map<std::string, Measurement> read() override {
    std::map<std::string, Measurement> data;
    int8_t dBm = WiFi.RSSI();
    float quality = getWifiQuality(dBm);
    data["wifi_dbm"] = {(float)dBm, "dBm"};
    data["wifi_quality"] = {quality, "%"};
    return data;
  }

 private:
  // Converts dBm to a range between 0 and 100%
  // From
  // https://github.com/ThingPulse/espaper-weatherstation/blob/master/espaper-weatherstation.ino
  int8_t getWifiQuality(int8_t dbm) {
    if (dbm <= -100) {
      return 0;
    } else if (dbm >= -50) {
      return 100;
    } else {
      return 2 * (dbm + 100);
    }
  }
};

#endif
