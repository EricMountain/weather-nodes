
#include <fmt/core.h>

#include "model.h"
#include "config.h"

Model::Model()
{
    doc_ = new JsonDocument();
    (*doc_)["nodes"] = JsonDocument();
}

Model::Model(const std::string &json_str)
{
    doc_ = new JsonDocument();
    jsonLoadOK_ = fromJsonString(json_str);
}

void Model::setDateTime(const std::string &datetime_str)
{
    (*doc_)["datetime"] = datetime_str;
}

std::string Model::getDateTime() const
{
    if (doc_->operator[]("datetime").is<std::string>())
    {
        return (*doc_)["datetime"].as<std::string>();
    }
    return "";
}

void Model::setSunInfo(const std::string &sunrise, const std::string &transit, const std::string &sunset)
{
    (*doc_)["sun"]["transit"] = transit;
    (*doc_)["sun"]["rise"] = sunrise;
    (*doc_)["sun"]["set"] = sunset;
}

std::string Model::getSunRise() const
{
    return get("sun", "rise");
}

std::string Model::getSunSet() const
{
    return get("sun", "set");
}

std::string Model::getSunTransit() const
{
    return get("sun", "transit");
}

void Model::setMoonInfo(const std::string &phase, const std::string &phase_letter, const std::string &rise, const std::string &transit, const std::string &set)
{
    (*doc_)["moon"]["phase"] = phase;
    (*doc_)["moon"]["phase_letter"] = phase_letter;
    (*doc_)["moon"]["rise"] = rise;
    (*doc_)["moon"]["transit"] = transit;
    (*doc_)["moon"]["set"] = set;
}

std::string Model::getMoonRise() const
{
    return get("moon", "rise");
}

std::string Model::getMoonSet() const
{
    return get("moon", "set");
}

std::string Model::getMoonTransit() const
{
    return get("moon", "transit");
}

std::string Model::getMoonPhase() const
{
    return get("moon", "phase");
}

char Model::getMoonPhaseLetter() const
{
    return get("moon", "phase_letter")[0];
}

void Model::addNode(JsonPair &raw_node, DateTime &utc_timestamp)
{
    JsonObject new_node = JsonObject();

    JsonObject raw_node_data = raw_node.value().as<JsonObject>();

    JsonString battery_level = JsonString();
    JsonString node_name = raw_node.key();
    JsonString displayName = node_name;
    if (raw_node_data["display_name"].is<JsonString>())
        displayName = raw_node_data["display_name"].as<JsonString>();
    new_node["display_name"] = displayName;

    addNodeBatteryLevel(raw_node_data, battery_level, new_node);
    addNodeStatusSection(raw_node_data, new_node);
    addNodeStaleState(utc_timestamp, raw_node_data, new_node);
    addNodeMeasurementsV2(raw_node_data, new_node);

    (*doc_)["nodes"][node_name] = new_node;
}

void Model::addNodeMeasurementsV2(ArduinoJson::V742PB22::JsonObject &raw_node_data, ArduinoJson::V742PB22::JsonObject &new_node)
{
    // Copy measurements_v2 if it exists, round values to one decimal place, ignore wifi
    // and battery sections
    if (raw_node_data["measurements_v2"].is<JsonObject>())
    {
        JsonObject measurements_v2 = raw_node_data["measurements_v2"].as<JsonObject>();
        for (JsonPair measurement : measurements_v2)
        {
            if (measurement.key() != "wifi" && measurement.key() != "battery")
            {
                double value = measurement.value().as<JsonFloat>();
                new_node["measurements_v2"][measurement.key()] = fmt::format(R"({:.1f})", value);
            }
        }
    }
}

void Model::addNodeStaleState(DateTime &utc_timestamp, JsonObject &raw_node_data, JsonObject &new_node)
{
    std::string node_stale = "";
    if (utc_timestamp.ok())
    {
        if (raw_node_data["timestamp_utc"].is<JsonString>())
        {
            std::string measurements_timestamp_utc = raw_node_data["timestamp_utc"].as<String>().c_str();
            struct tm tm_node_utc;
            DateTime node_utc_dt = DateTime(measurements_timestamp_utc);
            if (node_utc_dt.ok())
            {
                double diff = utc_timestamp.diff(node_utc_dt);
                if (diff < 0)
                {
                    node_stale = fmt::format("Time travel {:.0f}\"!", -diff);
                }
                else if (diff > MAX_STALE_SECONDS)
                {
                    node_stale = fmt::format("{:.0f}ʼ old", diff / 60);
                }
            }
            else
            {
                node_stale = fmt::format("(TS:{})", measurements_timestamp_utc.c_str());
                Serial.printf("Bad timestamp: %s\n", measurements_timestamp_utc.c_str());
            }
        }
    }
    else
    {
        node_stale = "(No reference time)";
    }
    new_node["stale_data"] = node_stale;
}

void Model::addNodeStatusSection(ArduinoJson::V742PB22::JsonObject &raw_node_data, ArduinoJson::V742PB22::JsonObject &new_node)
{
    if (raw_node_data["status"].is<JsonObject>())
    {
        new_node["status"] = raw_node_data["status"].as<JsonObject>();
    }
}

void Model::addNodeBatteryLevel(JsonObject &raw_node_data, JsonString &battery_level, JsonObject &new_node)
{
    if (raw_node_data["measurements_v2"].is<JsonObject>())
    {
        JsonObject measurements_v2 = raw_node_data["measurements_v2"].as<JsonObject>();
        if (measurements_v2["battery"].is<JsonObject>())
        {
            JsonObject battery = measurements_v2["battery"].as<JsonObject>();
            if (battery["battery_percentage"].is<JsonString>())
            {
                float battery_percentage = float(battery["battery_percentage"]);
                battery_level = JsonString(std::string{batteryLevelToChar(battery_percentage)}.c_str());
                new_node["battery_level"] = battery_level;
            }
        }
    }
}

JsonObject Model::getNodeData() const
{
    if ((*doc_)["nodes"].is<JsonObject>())
    {
        return (*doc_)["nodes"].as<JsonObject>();
    }
    return JsonObject();
}

void Model::addNodesData(JsonObject rawNodes)
{
    for (JsonPair node : rawNodes)
    {
        JsonObject nodeData = node.value().as<JsonObject>();

        if (nodeData["timestamp_utc"].is<JsonString>())
        {
            nodeData.remove("timestamp_utc");
        }

        if (nodeData["measurements_v2"].is<JsonObject>())
        {
            JsonObject measurements_v2 = nodeData["measurements_v2"].as<JsonObject>();

            // Eliminate Wifi because it fluctuates and we’re not really interested in
            // battery changes to the point of updating the display. Temperature alone
            // will be enough to drive that.
            if (measurements_v2["wifi"].is<JsonObject>())
            {
                measurements_v2.remove("wifi");
            }
            // if (measurements_v2["battery"].is<JsonObject>()) {
            //     measurements_v2.remove("battery");
            // }

            // Round everything else to one tenth precision
            for (JsonPair device : measurements_v2)
            {
                JsonObject metrics = device.value().as<JsonObject>();
                for (JsonPair metric : metrics)
                {
                    double value = metric.value().as<JsonFloat>();
                    metrics[metric.key()] = fmt::format(R"({:.1f})", value);
                }
            }
        }
    }
    (*doc_)["nodes"] = rawNodes;
}

std::string Model::toJsonString() const
{
    std::string output;
    serializeJson(*doc_, output);
    return output;
}

bool Model::fromJsonString(const std::string &json_str)
{
    DeserializationError error = deserializeJson(*doc_, json_str);
    return !error;
}

bool Model::operator==(const Model &other) const
{
    return toJsonString() == other.toJsonString();
}

bool Model::operator!=(const Model &other) const
{
    return !(*this == other);
}

std::string Model::get(std::string key, std::string subkey) const
{
    if (doc_->operator[](key).is<JsonObject>())
    {
        JsonObject obj = (*doc_)[key].as<JsonObject>();
        if (obj[subkey].is<std::string>())
        {
            return obj[subkey].as<std::string>();
        }
    }
    return "";
}

char Model::batteryLevelToChar(float battery_percentage)
{
    // List of characters for battery indicator, from empty to full
    char battery_chars[] = {'0', '5', '6', '7', '8', '9', ':', ';', '<'};
    int num_levels = sizeof(battery_chars) / sizeof(battery_chars[0]);
    float level = battery_percentage / 100.0 * (num_levels - 1);

    if (level < 0)
        level = 0;
    if (level >= num_levels)
        level = num_levels - 1;
    int char_offset = round(level);
    Serial.printf("Battery percentage: %.1f%%, level: %.1f, char_offset: %d\n", battery_percentage, level, char_offset);

    return battery_chars[char_offset];
}