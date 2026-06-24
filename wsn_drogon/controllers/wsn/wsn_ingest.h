#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace wsn
{
class ingest : public drogon::HttpController<ingest>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(wsn::ingest::receive, "/wsn/api/ingest/receive", Post);
    METHOD_LIST_END

   void receive(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback);
};
}