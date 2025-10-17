#include "display_view.h"

bool DisplayView::buildModel(JsonDocument* doc,
                             const std::map<std::string, Sensor*>& sensors) {
  doc_ = doc;
  sensors_ = sensors;

  // Force render if we have no data
  if (doc_ == nullptr || doc_->isNull() || !(*doc_)["nodes"].is<JsonObject>()) {
    return true;
  }

  utc_timestamp_ = parseTimestampValue("timestamp_utc");
  local_timestamp_ = parseTimestampValue("timestamp_local");

  model_.buildFromJson(doc_, utc_timestamp_, local_timestamp_);

  Controller c = Controller(model_);
  return c.needRefresh();
  // return true;
}

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
