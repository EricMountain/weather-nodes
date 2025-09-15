#pragma once

#include <LittleFS.h>

#include "model.h"

class Controller
{
public:
    // TODO move code to .cpp
    Controller(Model &current) : current_(current) {
        LittleFS.begin(true);
        if (LittleFS.exists(dataFilePath)) {
            File file = LittleFS.open(dataFilePath, "r");
            if (file) {
                std::string json_str;
                while (file.available()) {
                    json_str += (char)file.read();
                }
                lastDisplayed_ = Model(json_str);
                if (!lastDisplayed_.jsonLoadOK()) {
                    Serial.println("Failed to parse last displayed model from file");
                }
            }
        } else {
            Serial.println("No last displayed model file found");
        }
        LittleFS.end();
        
        Serial.printf("Last displayed model: %s\n", lastDisplayed_.toJsonString().c_str());

        needRefresh_ = !lastDisplayed_.jsonLoadOK() || (lastDisplayed_ != current_);
        if (needRefresh_) {
            Serial.println("Current model differs from last displayed model, need refresh");
        } else {
            Serial.println("Current model matches last displayed model, no refresh needed");
        }

        // TODO remove when ready
        Serial.println("Forcing refresh for now");
        needRefresh_ = true; // always refresh for now

        writeData();
    }

    bool needRefresh() const { return needRefresh_; }

private:
    const char* dataFilePath = "/last-displayed.json";
    Model lastDisplayed_;
    Model &current_;
    bool needRefresh_ = true;

    // TODO move code to .cpp
    void writeData() {
        if (!needRefresh_) {
            Serial.println("No refresh needed, skipping write");
            return;
        }
        Serial.printf("Writing current model to file: %s\n", current_.toJsonString().c_str());

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
};
