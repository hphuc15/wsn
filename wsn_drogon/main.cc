#include <drogon/drogon.h>
#include "mqtt/MqttManager.h"

int main() {
    drogon::app().addListener("0.0.0.0", 5555);
    drogon::app().loadConfigFile("../config.json");

    MqttManager::instance().init("tcp://localhost:1883", "wsn_server");

    drogon::app().registerBeginningAdvice([]() {
        MqttManager::instance().start();
    });

    drogon::app().run();
    return 0;
}