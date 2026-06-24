#pragma once
#include <drogon/plugins/Plugin.h>

class WsnPlugin : public drogon::Plugin<WsnPlugin> {
public:
    void initAndStart(const Json::Value& config) override;
    void shutdown() override;
};