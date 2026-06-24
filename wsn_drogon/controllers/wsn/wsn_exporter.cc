#include "wsn_exporter.h"
#include "wsn_telemetry_util.h"
#include <drogon/drogon.h>
#include <drogon/orm/DbClient.h>
#include <sstream>
#include <regex>

using namespace wsn;
using namespace drogon::orm;

static bool isValidDatetime(const std::string &s)
{
    static const std::regex re(R"(^\d{4}-\d{2}-\d{2}([ T]\d{2}:\d{2}:\d{2})?$)");
    return std::regex_match(s, re);
}

/**
 * @brief Export CSV api
 * @note method: GET /wsn/api/exporter/csv
 */
void wsn::exporter::csv(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto device_id_str = req->getParameter("device_id");
    auto from          = req->getParameter("from");
    auto to            = req->getParameter("to");
    auto limit_str     = req->getParameter("limit");

    int lim = 1000;
    if (!limit_str.empty()) {
        try {
            lim = std::stoi(limit_str);
            if (lim <= 0 || lim > 10000) lim = 1000;
        } catch (...) {
            lim = 1000;
        }
    }

    bool has_device = false;
    int  device_id  = 0;
    if (!device_id_str.empty()) {
        try {
            device_id  = std::stoi(device_id_str);
            has_device = true;
        } catch (...) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k400BadRequest);
            resp->setBody("Invalid device_id");
            callback(resp);
            return;
        }
    }

    bool has_range = !from.empty() && !to.empty();
    if (has_range && (!isValidDatetime(from) || !isValidDatetime(to))) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid from/to format, expected YYYY-MM-DD or YYYY-MM-DD HH:MM:SS");
        callback(resp);
        return;
    }

    auto db = drogon::app().getDbClient("wsn");

    std::ostringstream sql;
    sql << "SELECT device_id, ts, temp, humi, soil_humi FROM telemetry WHERE 1=1";
    if (has_device) sql << " AND device_id = " << device_id;
    if (has_range)  sql << " AND ts BETWEEN '" << from << "' AND '" << to << "'";
    sql << " ORDER BY ts DESC LIMIT " << lim;

    db->execSqlAsync(sql.str(),
        [callback](const Result &r) {
            std::ostringstream oss;
            oss << "device_id,time,temp,humi,soil_humi\r\n";

            for (auto &row : r) {
                auto t = rowToTelemetry(row);
                oss << t.device_id << "," << t.ts << ",";
                t.temp      ? oss << *t.temp      : oss << "";
                oss << ",";
                t.humi      ? oss << *t.humi      : oss << "";
                oss << ",";
                t.soil_humi ? oss << *t.soil_humi : oss << "";
                oss << "\r\n";
            }

            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k200OK);
            resp->setContentTypeCode(CT_CUSTOM);
            resp->addHeader("Content-Type", "text/csv; charset=utf-8");
            resp->addHeader("Content-Disposition", "attachment; filename=\"sensor_export.csv\"");
            resp->setBody(oss.str());
            callback(resp);
        },
        [callback](const DrogonDbException &e) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(e.base().what());
            callback(resp);
        }
    );
}