#pragma once

#include <string>
#include <map>
#include <sstream>
#include <algorithm>


#include <ArduinoJson.h>

class Model
{
public:
    Model();
    Model(const std::string &json_str);
    bool jsonLoadOK() const { return jsonLoadOK_; }
    void setDateTime(const std::string &datetime_str);
    std::string getDateTime() const;
    void setSunInfo(const std::string &sunrise, const std::string &transit, const std::string &sunset);
    std::string getSunRise() const;
    std::string getSunSet() const;
    std::string getSunTransit() const;
    void setMoonInfo(const std::string &phase, const std::string &phase_letter, const std::string &rise, const std::string &transit, const std::string &set);
    std::string getMoonRise() const;
    std::string getMoonSet() const;
    std::string getMoonTransit() const;
    std::string getMoonPhase() const;
    char getMoonPhaseLetter() const;
    void addNodesData(JsonObject nodes);
    std::string toJsonString() const;
    bool fromJsonString(const std::string &json_str);
    bool operator==(const Model &other) const;
    bool operator!=(const Model &other) const;

private:
    JsonDocument *doc_;
    bool jsonLoadOK_ = false;
    std::string get(std::string key, std::string subkey) const;
};
