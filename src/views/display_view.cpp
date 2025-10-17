#include "display_view.h"

DateTime DisplayView::parseTimestampValue(const String& timestamp_key) {
  DateTime dt;

  if ((*doc_)[timestamp_key].is<JsonString>()) {
    std::string timestamp = (*doc_)[timestamp_key].as<String>().c_str();
    dt = parseTimestamp(timestamp, timestamp_key);
  }
  return dt;
}

DateTime DisplayView::parseTimestamp(const std::string& timestamp,
                                     const String& timestamp_key) {
  DateTime dt(timestamp);
  if (!dt.ok()) {
    Serial.printf("Failed to parse %s: %s\n", timestamp_key.c_str(),
                  timestamp.c_str());
  }
  return dt;
}
