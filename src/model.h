#pragma once

#include <string>
#include <map>
#include <sstream>
#include <algorithm>

#include <ArduinoJson.h>

class Model
{
public:
    Model() {
        doc_ = new JsonDocument();
    }

    Model(const std::string &json_str) {
        doc_ = new JsonDocument();
        jsonLoadOK_ = fromJsonString(json_str);
    }

    bool jsonLoadOK() const { return jsonLoadOK_; }

    void setDateTime(const std::string& datetime_str) {
        (*doc_)["datetime"] = datetime_str;
    }

    std::string getDateTime() const {
        if (doc_->operator[]("datetime").is<std::string>()) {
            return (*doc_)["datetime"].as<std::string>();
        }
        return "";
    }

    std::string toJsonString() const {
        std::string output;
        serializeJson(*doc_, output);
        return output;
    }

    bool fromJsonString(const std::string& json_str) {
        DeserializationError error = deserializeJson(*doc_, json_str);
        return !error;
    }

    bool operator==(const Model& other) const {
        return toJsonString() == other.toJsonString();
    }

    bool operator!=(const Model& other) const {
        return !(*this == other);
    }

private:
    JsonDocument* doc_;
    bool jsonLoadOK_ = false;
    
};
