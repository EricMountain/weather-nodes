#ifndef BME680_H
#define BME680_H

#include <Adafruit_BME680.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

#include "config.h"
#include "sensor.h"

class BME680Sensor : public Sensor {
 public:
  BME680Sensor(uint8_t i2c_addr = BME680_I2C_ADDR) : bme() {}

  bool init() override {
    if ((ok_ = bme.begin(BME680_I2C_ADDR))) {
      bme.setTemperatureOversampling(BME680_OS_8X);
      bme.setHumidityOversampling(BME680_OS_2X);
      bme.setPressureOversampling(BME680_OS_4X);
      bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
#ifdef BME680_ENABLE_GAS_HEATER
      bme.setGasHeater(320, 150);  // 320°C for 150 ms
#endif
      Serial.println("BME680 sensor initialized");
    } else {
      Serial.println("Could not find a valid BME680 sensor, check wiring!");
    }
    return ok_;
  }

  bool ok() const override { return ok_; }

  std::map<std::string, Measurement> read() override {
    std::map<std::string, Measurement> data;
    if (bme.performReading()) {
      data["temperature"] = {
          bme.temperature + (float)BME680_TEMPERATURE_CORRECTION, "C"};
      data["humidity"] = {bme.humidity, "%"};
      data["pressure"] = {bme.pressure / 100.0f, "hPa"};
#ifdef BME680_ENABLE_GAS_HEATER
      data["gas_resistance"] = {static_cast<float>(bme.gas_resistance), "Ohms"};
#endif
    }
    return data;
  }

 private:
  Adafruit_BME680 bme;
  bool ok_ = false;
};

#endif
