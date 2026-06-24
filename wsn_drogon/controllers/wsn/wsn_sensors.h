#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace wsn
{
class sensors : public drogon::HttpController<sensors>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(wsn::sensors::getAll,        "/wsn/api/v1/sensors",                      Get);
    ADD_METHOD_TO(wsn::sensors::getLatest,     "/wsn/api/v1/sensors/{device_id}",          Get);
    ADD_METHOD_TO(wsn::sensors::getHistory,    "/wsn/api/v1/sensors/{device_id}/history",  Get);
    METHOD_LIST_END

    void getAll     (const HttpRequestPtr &req, std::function<void (const HttpResponsePtr&)> &&callback);
    void getLatest  (const HttpRequestPtr &req, std::function<void (const HttpResponsePtr&)> &&callback, int device_id);
    void getHistory (const HttpRequestPtr &req, std::function<void (const HttpResponsePtr&)> &&callback, int device_id);
};
}
