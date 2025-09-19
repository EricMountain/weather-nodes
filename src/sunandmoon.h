#ifndef SUN_AND_MOON_H
#define SUN_AND_MOON_H

#include <SunMoonCalc.h>

#include <cmath>
#include <string>

#include "datetime.h"

class SunAndMoon {
 public:
  SunAndMoon(int year, int month, int day, int hour, int minute, int second,
             double latitude, double longitude, int utc_offset_seconds)
      : year(year),
        month(month),
        day(day),
        hour(hour),
        minute(minute),
        second(second),
        latitude(latitude),
        longitude(longitude),
        utc_offset_seconds(utc_offset_seconds),
        sunMoonCalc(year, month, day, hour, minute, second, latitude,
                    longitude) {
    sunMoonCalcResult = sunMoonCalc.calculateSunAndMoonData();
  }

  std::string getSunrise() {
    time_t local = sunMoonCalcResult.sun.rise + utc_offset_seconds;
    return formatTime(local);
  };

  std::string getSunset() {
    time_t local = sunMoonCalcResult.sun.set + utc_offset_seconds;
    return formatTime(local);
  };

  std::string getSunTransit() {
    time_t local = sunMoonCalcResult.sun.transit + utc_offset_seconds;
    return formatTime(local);
  }

  std::string getMoonRise() {
    time_t local = sunMoonCalcResult.moon.rise + utc_offset_seconds;
    return formatTime(local);
  };

  std::string getMoonSet() {
    time_t local = sunMoonCalcResult.moon.set + utc_offset_seconds;
    return formatTime(local);
  }

  std::string getMoonTransit() {
    time_t local = sunMoonCalcResult.moon.transit + utc_offset_seconds;
    return formatTime(local);
  }

  std::string getMoonPhase() {
    return sunMoonCalcResult.moon.phase.name.c_str();
  }

  double getMoonPhaseAge() { return sunMoonCalcResult.moon.age; }

  char getMoonPhaseLetter() {
    double lunarAge = sunMoonCalcResult.moon.age;
    if (lunarAge >= 0 && lunarAge <= lunar_cycle_days &&
        (lunarAge < 1 || lunarAge > lunar_cycle_days - 1)) {
      return '0';  // New Moon
    }

    int phases = 26;
    int index = (int)round((lunarAge / lunar_cycle_days) * phases) % phases;
    return 'A' + index;
  }

 private:
  SunMoonCalc sunMoonCalc;
  SunMoonCalc::Result sunMoonCalcResult;
  int year;
  int month;
  int day;
  int hour;
  int minute;
  int second;
  double latitude;
  double longitude;
  int utc_offset_seconds;
  const double lunar_cycle_days = 29.530588853;

  std::string formatTime(time_t time) {
    DateTime dt(time);
    return dt.format("%H:%M");
  }
};

#endif  // SUN_H