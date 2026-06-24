#include "MqttManager.h"
#include <trantor/utils/Logger.h>
#include <sstream>

MqttManager& MqttManager::instance() {
    static MqttManager inst;
    return inst;
}

void MqttManager::init(const std::string& broker_address,
                       const std::string& client_id) {
    client_ = std::make_unique<mqtt::async_client>(broker_address, client_id);
    conn_opts_.set_keep_alive_interval(20);
    conn_opts_.set_clean_session(true);
    conn_opts_.set_automatic_reconnect(true);
    client_->set_callback(*this);
    LOG_INFO << "MqttManager initialized: " << broker_address;
}

void MqttManager::start() {
    try {
        LOG_INFO << "MQTT connecting...";
        client_->connect(conn_opts_)->wait();
    } catch (const mqtt::exception& e) {
        LOG_ERROR << "MQTT connect failed: " << e.what();
    }
}

void MqttManager::addHandler(const std::string& topic_filter,
                              MqttMessageHandler handler) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    handlers_.push_back({topic_filter, handler});
    subscribed_filters_.push_back(topic_filter);
    LOG_INFO << "MqttManager: registered handler for [" << topic_filter << "]";
}

void MqttManager::connected(const std::string& cause) {
    LOG_INFO << "MQTT connected. Subscribing all registered topics...";
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    for (const auto& h : handlers_) {
        client_->subscribe(h.topic_filter, 1);
        LOG_INFO << "  -> subscribed: " << h.topic_filter;
    }
}

void MqttManager::connection_lost(const std::string& cause) {
    LOG_WARN << "MQTT connection lost: " << cause;
}

void MqttManager::message_arrived(mqtt::const_message_ptr msg) {
    std::string topic   = msg->get_topic();
    std::string payload = msg->get_payload_str();

    LOG_DEBUG << "MQTT [" << topic << "] " << payload;

    std::lock_guard<std::mutex> lock(handlers_mutex_);
    for (const auto& h : handlers_) {
        if (topicMatch(h.topic_filter, topic)) {
            h.handler(topic, payload);
        }
    }
}

bool MqttManager::topicMatch(const std::string& filter,
                              const std::string& topic) {
    // Tách filter và topic thành các level theo dấu "/"
    auto split = [](const std::string& s) {
        std::vector<std::string> parts;
        std::stringstream ss(s);
        std::string part;
        while (std::getline(ss, part, '/')) parts.push_back(part);
        return parts;
    };

    auto f_parts = split(filter);
    auto t_parts = split(topic);

    for (size_t i = 0; i < f_parts.size(); ++i) {
        if (f_parts[i] == "#") return true;  // match tất cả phần còn lại
        if (i >= t_parts.size())  return false;
        if (f_parts[i] != "+" && f_parts[i] != t_parts[i]) return false;
    }
    return f_parts.size() == t_parts.size();
}