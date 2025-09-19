#include "datetime.h"

#include <fmt/core.h>

#include <ctime>
#include <Arduino.h>

DateTime::DateTime()
    : timestamp_(""),
      year_(0),
      month_(0),
      day_(0),
      hour_(0),
      minute_(0),
      second_(0) {
  memset(&timeinfo, 0, sizeof(timeinfo));
}

DateTime::DateTime(std::string timestamp) {
  memset(&timeinfo, 0, sizeof(timeinfo));
  // NB: %z breaks day of week calculation and TZ offset is not parsed
  if (strptime(timestamp.c_str(), "%Y-%m-%dT%H:%M:%S", &timeinfo)) {
    timestamp_ = timestamp;
    year_ = timeinfo.tm_year + 1900;
    month_ = timeinfo.tm_mon + 1;
    day_ = timeinfo.tm_mday;
    hour_ = timeinfo.tm_hour;
    minute_ = timeinfo.tm_min;
    second_ = timeinfo.tm_sec;
  } else {
    // Handle invalid timestamp format
    year_ = month_ = day_ = hour_ = minute_ = second_ = 0;
  }
}

bool DateTime::ok() { return year_ != 0; }

std::string DateTime::niceDate() {
  std::string weekday = safe_strftime("%A", &timeinfo);
  std::string month = safe_strftime("%B", &timeinfo);
  return fmt::format("{} {}{} {} {}", weekday, day_, dateSuffix(), month,
                     year_);
}

// From https://stackoverflow.com/a/58726549
// Modified to not use make_unique as we don’t have it in this version of C++
std::string DateTime::safe_strftime(const char *fmt, const tm *t) {
  std::size_t len = 30;
  char *buff = new char[len];
  while (std::strftime(buff, len, fmt, t) == 0) {
    delete[] buff;
    len *= 2;
    buff = new char[len];
  }
  std::string result(buff);
  delete[] buff;
  return result;
}

const char *DateTime::dateSuffix() const {
  if (day_ >= 11 && day_ <= 13) {
    return "th";
  }
  switch (day_ % 10) {
    case 1:
      return "st";
    case 2:
      return "nd";
    case 3:
      return "rd";
    default:
      return "th";
  }
}
