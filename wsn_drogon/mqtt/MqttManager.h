#pragma once

#include <mqtt/async_client.h>
#include <drogon/drogon.h>
#include <string>
#include <functional>
#include <vector>
#include <mutex>

using MqttMessageHandler = std::function<void(const std::string& topic, const std::string& payload)>;

struct MqttTopicHandler {
    std::string        topic_filter;
    MqttMessageHandler handler;
};

class MqttManager : public mqtt::callback {
public:
    /* Singleton */
    static MqttManager& instance();

    void init(const std::string& broker_address,
              const std::string& client_id);
    void start();

    // Plugin call this function to register handler
    void addHandler(const std::string& topic_filter, MqttMessageHandler handler);

private:
    MqttManager() = default;

    /* Override mqtt::callback */
    void connected(const std::string& cause) override;
    void connection_lost(const std::string& cause) override;
    void message_arrived(mqtt::const_message_ptr msg) override;

    // Match topic with filter (support wildcard # và +)
    bool topicMatch(const std::string& filter, const std::string& topic);

    std::unique_ptr<mqtt::async_client> client_;
    mqtt::connect_options               conn_opts_;
    std::vector<MqttTopicHandler>       handlers_;
    std::mutex                          handlers_mutex_;
    std::vector<std::string>            subscribed_filters_;
};