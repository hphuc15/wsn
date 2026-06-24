#pragma once

#include <json/json.h>
#include <drogon/orm/Result.h>
#include <optional>

namespace wsn
{

struct TelemetryRow {
    int                   device_id;
    std::string           ts;
    std::optional<float>  temp;
    std::optional<float>  humi;
    std::optional<float>  soil_humi;
};

inline TelemetryRow rowToTelemetry(const drogon::orm::Row &row)
{
    TelemetryRow t;
    t.device_id = row["device_id"].as<int>();
    t.ts        = row["ts"].as<std::string>();

    if (!row["temp"].isNull())      t.temp      = row["temp"].as<float>();
    if (!row["humi"].isNull())      t.humi      = row["humi"].as<float>();
    if (!row["soil_humi"].isNull()) t.soil_humi = row["soil_humi"].as<float>();

    return t;
}

inline Json::Value telemetryToJson(const TelemetryRow &t)
{
    Json::Value v;
    v["device_id"] = t.device_id;
    v["ts"]        = t.ts;

    if (t.temp)      v["temp"]      = *t.temp;
    if (t.humi)      v["humi"]      = *t.humi;
    if (t.soil_humi) v["soil_humi"] = *t.soil_humi;

    return v;
}

}