#include "wsn_sensors.h"
#include "wsn_telemetry_util.h"
#include <drogon/drogon.h>
#include <drogon/orm/DbClient.h>

using namespace wsn;
using namespace drogon::orm;

/**
 * @brief Get all data of device
 * @note method: GET /wsn/api/v1/sensors
 */
void wsn::sensors::getAll(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto db = drogon::app().getDbClient("wsn");

    /* Get newest row (follow ts) of each device */
    const std::string sql =
        "SELECT t.device_id, t.ts, t.temp, t.humi, t.soil_humi "
        "FROM telemetry t "
        "INNER JOIN ("
        "    SELECT device_id, MAX(ts) AS max_ts"
        "    FROM telemetry"
        "    GROUP BY device_id"
        ") latest ON t.device_id = latest.device_id AND t.ts = latest.max_ts "
        "ORDER BY t.device_id";

    db->execSqlAsync(sql,
        [callback](const Result &r) {
            Json::Value result(Json::arrayValue);
            for (auto &row : r)
                result.append(telemetryToJson(rowToTelemetry(row)));
            callback(HttpResponse::newHttpJsonResponse(result));
        },
        [callback](const DrogonDbException &e) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(e.base().what());
            callback(resp);
        }
    );
}

/**
 * @brief Get latest record of device
 * @note method: GET /wsn/api/v1/sensors/{device_id}GET /wsn/api/v1/sensors/{device_id}
 */
void wsn::sensors::getLatest(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int device_id)
{
    auto db = drogon::app().getDbClient("wsn");

    const std::string sql =
        "SELECT device_id, ts, temp, humi, soil_humi "
        "FROM telemetry "
        "WHERE device_id = ? "
        "ORDER BY ts DESC LIMIT 1";

    db->execSqlAsync(sql,
        [callback](const Result &r) {
            if (r.size() == 0) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k404NotFound);
                callback(resp);
                return;
            }
            callback(HttpResponse::newHttpJsonResponse(telemetryToJson(rowToTelemetry(r[0]))));
        },
        [callback](const DrogonDbException &e) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(e.base().what());
            callback(resp);
        },
        device_id
    );
}

/**
 * @brief Get telemetry history of device
 * @note method: GET /wsn/api/v1/sensors/{device_id}/history
 */
void wsn::sensors::getHistory(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int device_id)
{
    auto from  = req->getParameter("from");
    auto to    = req->getParameter("to");
    auto limit = req->getParameter("limit");

    int lim = 200;
    if (!limit.empty()) {
        try {
            lim = std::stoi(limit);
            if (lim <= 0 || lim > 1000) lim = 200;
        } catch (...) {
            lim = 200;
        }
    }

    auto db = drogon::app().getDbClient("wsn");

    auto handler = [callback](const Result &r) {
        Json::Value result(Json::arrayValue);
        for (auto &row : r)
            result.append(telemetryToJson(rowToTelemetry(row)));
        callback(HttpResponse::newHttpJsonResponse(result));
    };

    auto exception_handler = [callback](const DrogonDbException &e) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody(e.base().what());
        callback(resp);
    };

    if (!from.empty() && !to.empty()) {
        const std::string sql =
            "SELECT device_id, ts, temp, humi, soil_humi "
            "FROM telemetry "
            "WHERE device_id = ? AND ts BETWEEN ? AND ? "
            "ORDER BY ts DESC LIMIT ?";
        db->execSqlAsync(sql, handler, exception_handler, device_id, from, to, lim);
    } else {
        const std::string sql =
            "SELECT device_id, ts, temp, humi, soil_humi "
            "FROM telemetry "
            "WHERE device_id = ? "
            "ORDER BY ts DESC LIMIT ?";
        db->execSqlAsync(sql, handler, exception_handler, device_id, lim);
    }
}