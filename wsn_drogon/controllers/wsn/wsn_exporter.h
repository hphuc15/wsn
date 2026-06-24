#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace wsn
{
class exporter : public drogon::HttpController<exporter>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(wsn::exporter::csv, "/wsn/api/exporter/csv", Get);
    METHOD_LIST_END

    void csv(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
};
}