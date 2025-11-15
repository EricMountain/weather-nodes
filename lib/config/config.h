#ifndef CONFIG_H
#define CONFIG_H

#define MAX_STALE_SECONDS 60 * 30

#define BME680_I2C_ADDR 0x77
// #define BME680_ENABLE_GAS_HEATER
#define BME680_TEMPERATURE_CORRECTION -1.8

#define SHT31D_I2C_ADDR 0x44

// Devices
#ifdef INDOOR_DISPLAY_NODE
#define USE_THINGPULSE_EPULSE_FEATHER
#define HAS_DISPLAY
#define LIGHT_SLEEP_ENABLED
// #define FORCE_DEEP_SLEEP
// #define SLEEP_SECONDS 900
#define SLEEP_SECONDS 120
#define HAS_BME680
#define HAS_BATTERY
#define OTA_UPDATE_ENABLED
#define DISPLAY_NODE_VERSIONS
// #define DISPLAY_TIME
#endif

#ifdef OUTDOOR_NODE
#define HAS_SHT31D
#define HAS_BATTERY
#define OTA_UPDATE_ENABLED
#endif

#ifdef PROTOTYPE_NODE
#define HAS_SHT31D
#define HAS_BATTERY
#define OTA_UPDATE_ENABLED
#endif

#ifdef DUMMY_NODE
#define OTA_UPDATE_ENABLED
#endif

#ifdef HAS_BATTERY
// Battery voltage calculation
#define BAT_MON_PIN 35
#define R1 2.2f
#define R2 4.7f
#endif

#ifndef SLEEP_SECONDS
#ifdef HAS_BATTERY
// Rough estimate of battery life is 10 days per minute of sleep
#define SLEEP_SECONDS 60 * 10
#else
#define SLEEP_SECONDS 60 * 2
#endif
#endif

#endif
