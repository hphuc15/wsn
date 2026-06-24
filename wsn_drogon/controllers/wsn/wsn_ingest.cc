#include "wsn_ingest.h"
#include <drogon/drogon.h>
#include <drogon/orm/DbClient.h>

using namespace wsn;
using namespace drogon::orm;

/**
 * @brief Receive data api
 * @note method: POST /wsn/api/ingest/receive
 */
void wsn::ingest::receive(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json || !json->isObject() || !json->isMember("device_id")) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Missing device_id or invalid JSON");
        callback(resp);
        return;
    }

    int device_id = (*json)["device_id"].asInt();

    /* Read field; NULL if not exists in payload */
    std::optional<float> temp, humi, soil_humi;

    if (json->isMember("temp")      && !(*json)["temp"].isNull())
        temp      = (*json)["temp"].asFloat();
    if (json->isMember("humi")      && !(*json)["humi"].isNull())
        humi      = (*json)["humi"].asFloat();
    if (json->isMember("soil_humi") && !(*json)["soil_humi"].isNull())
        soil_humi = (*json)["soil_humi"].asFloat();

    /* At least one field must be measured */
    if (!temp && !humi && !soil_humi) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("No telemetry fields found (expected: temp, humi, soil_humi)");
        callback(resp);
        return;
    }

    auto db = drogon::app().getDbClient("wsn");

    const std::string sql =
        "INSERT INTO telemetry (device_id, temp, humi, soil_humi) "
        "VALUES (?, ?, ?, ?)";

    db->execSqlAsync(sql,
        [callback](const Result &) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k200OK);
            callback(resp);
        },
        [callback](const DrogonDbException &e) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(e.base().what());
            callback(resp);
        },
        device_id, temp, humi, soil_humi
    );
}