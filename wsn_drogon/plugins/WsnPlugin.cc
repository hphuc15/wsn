#include "WsnPlugin.h"
#include "../mqtt/MqttManager.h"
#include <trantor/utils/Logger.h>
#include <json/json.h>

void WsnPlugin::initAndStart(const Json::Value& config) {
    LOG_INFO << "WsnPlugin starting...";

    MqttManager::instance().addHandler(
        "sensors/#",
        [](const std::string& topic, const std::string& payload) {

            /* Parse device_id from topic "sensors/3" -> 3 */
            int device_id = std::stoi(topic.substr(topic.find_last_of('/') + 1));

            /* Parse JSON payload */
            Json::Value root;
            Json::Reader reader;
            if (!reader.parse(payload, root)) {
                LOG_WARN << "WsnPlugin: invalid JSON from " << topic;
                return;
            }

            std::optional<float> temp, humi, soil_humi;

            if (root.isMember("temp") && !root["temp"].isNull()){ temp = root["temp"].asFloat(); }
            if (root.isMember("humi") && !root["humi"].isNull()){ humi = root["humi"].asFloat(); }
            if (root.isMember("soil_humi") && !root["soil_humi"].isNull()){ soil_humi = root["soil_humi"].asFloat(); }
            if (!temp && !humi && !soil_humi) {
                LOG_WARN << "WsnPlugin: no telemetry fields in payload from " << topic;
                return;
            }
            /* Insert to database */
            auto db = drogon::app().getDbClient("wsn");
            db->execSqlAsync(
                "INSERT INTO telemetry (device_id, temp, humi, soil_humi) "
                "VALUES (?,?,?,?)",
                [](const drogon::orm::Result&) {
                    LOG_DEBUG << "WsnPlugin: insert OK";
                },
                [](const drogon::orm::DrogonDbException& e) {
                    LOG_ERROR << "WsnPlugin: insert failed: " << e.base().what();
                },
                device_id, temp, humi, soil_humi
            );
        }
    );
}

void WsnPlugin::shutdown() {
    LOG_INFO << "WsnPlugin shutdown";
}