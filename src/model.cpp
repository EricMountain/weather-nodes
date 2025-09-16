
#include <fmt/core.h>
#include "model.h"

Model::Model()
{
    doc_ = new JsonDocument();
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

void Model::addNodesData(JsonObject nodes)
{
    for (JsonPair node : nodes)
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
    (*doc_)["nodes"] = nodes;
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
