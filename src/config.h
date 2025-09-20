#ifndef CONFIG_H
#define CONFIG_H

// #define INDOOR_DISPLAY_NODE
// #define OUTDOOR_NODE
#define MAX_STALE_SECONDS 60 * 30

#define BME680_I2C_ADDR 0x77
// #define BME680_ENABLE_GAS_HEATER
#define BME680_TEMPERATURE_CORRECTION -1.8

#define SHT31D_I2C_ADDR 0x44

// Devices
#ifdef INDOOR_DISPLAY_NODE
#define HAS_DISPLAY
#define HAS_BME680
#endif

#ifdef OUTDOOR_NODE
#define HAS_SHT31D
#define HAS_BATTERY
#endif

#ifdef PROTOTYPE_NODE
#define HAS_SHT31D
#endif

#ifdef HAS_BATTERY
// Rough estimate of battery life is 10 days per minute of sleep
#define SLEEP_SECONDS 60 * 10
// Battery voltage calculation
#define BAT_MON_PIN 35
#define R1 2.2f
#define R2 4.7f
#else
#define SLEEP_SECONDS 60 * 2
#endif

#endif
