#ifndef SHT31D_H
#define SHT31D_H

#include <Adafruit_SHT31.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

#include "config.h"
#include "sensor.h"

class SHT31DSensor : public Sensor {
 public:
  SHT31DSensor() : sht31() {};

  bool init() override {
    if ((ok_ = sht31.begin(SHT31D_I2C_ADDR))) {
      Serial.println("SHT31D sensor initialized");
    } else {
      Serial.println("Could not find a valid SHT31D sensor, check wiring!");
    }
    return ok_;
  }

  bool ok() const override { return ok_; }

  std::map<std::string, Measurement> read() override {
    std::map<std::string, Measurement> data;
    if (ok_) {
      data["temperature"] = {sht31.readTemperature(), "C"};
      data["humidity"] = {sht31.readHumidity(), "%"};
    }
    return data;
  }

 private:
  Adafruit_SHT31 sht31;
  bool ok_ = false;
};

#endif
