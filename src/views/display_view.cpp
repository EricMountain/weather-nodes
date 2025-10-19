#include "display_view.h"

bool DisplayView::buildModel(JsonDocument* doc,
                             const std::map<std::string, Sensor*>& sensors) {
  sensors_ = sensors;

  // Only build model if we have a valid document
  if (doc == nullptr || doc->isNull() || !(*doc)["nodes"].is<JsonObject>()) {
    doc_is_valid_ = false;
    return true;
  }

  doc_is_valid_ = true;

  utc_timestamp_ = parseTimestampValue(doc, "timestamp_utc");
  local_timestamp_ = parseTimestampValue(doc, "timestamp_local");

  model_.setHttpPostErrorCode(http_post_error_code_);
  model_.setCurrentDeviceId(current_device_id_);
  model_.setTime(local_timestamp_.format("%H:%M:%S"));
  model_.buildFromJson(doc, utc_timestamp_, local_timestamp_);

  Controller c = Controller(model_);
  return c.needRefresh();
}

DateTime DisplayView::parseTimestampValue(JsonDocument* doc,
                                          const String& timestamp_key) {
  DateTime dt;

  if ((*doc)[timestamp_key].is<JsonString>()) {
    std::string timestamp = (*doc)[timestamp_key].as<String>().c_str();
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
