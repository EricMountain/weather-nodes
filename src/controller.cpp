#include "controller.h"

#include <LittleFS.h>

Controller::Controller(Model& current) : current_(current) {
  Model* lastDisplayed = nullptr;
  LittleFS.begin(true);
  if (LittleFS.exists(dataFilePath)) {
    File file = LittleFS.open(dataFilePath, "r");
    if (file) {
      std::string json_str;
      while (file.available()) {
        json_str += (char)file.read();
      }
      lastDisplayed = new Model(json_str);
      if (!lastDisplayed->jsonLoadOK()) {
        Serial.println("Failed to parse last displayed model from file");
      }
    }
  } else {
    Serial.println("No last displayed model file found");
  }
  LittleFS.end();

  Serial.printf("Last displayed model: %s\n",
                lastDisplayed->toJsonString().c_str());
  Serial.printf("lastDisplayed->jsonLoadOK(): %d\n",
                lastDisplayed->jsonLoadOK());
  Serial.printf("Model comparison result: %s\n",
                (*lastDisplayed == current_) ? "MATCH" : "DIFF");

  needRefresh_ = !lastDisplayed->jsonLoadOK() || !(*lastDisplayed == current_);
  if (needRefresh_) {
    Serial.println(
        "Current model differs from last displayed model, need refresh");
    // Only write if we refresh the screen - otherwise display and persisted
    // data may diverge without leading to a refresh
    writeData();
  } else {
    Serial.println(
        "Current model matches last displayed model, no refresh needed");
  }

  if (lastDisplayed != nullptr) {
    delete lastDisplayed;
  }
}

void Controller::writeData() {
  if (!needRefresh_) {
    Serial.println("No refresh needed, skipping write");
    return;
  }
  Serial.printf("Writing current model to file: %s\n",
                current_.toJsonString().c_str());

  LittleFS.begin(true);
  File file = LittleFS.open(dataFilePath, "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
    LittleFS.end();
    return;
  }
  file.print(current_.toJsonString().c_str());
  file.close();
  Serial.println("Current model written to file");

  Serial.println("Files in /:");
  File root = LittleFS.open("/");
  File f = root.openNextFile();
  while (f) {
    Serial.printf("  %s    %d bytes\n", f.name(), f.size());
    f = root.openNextFile();
  }

  LittleFS.end();
}
