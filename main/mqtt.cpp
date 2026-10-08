#include "mqtt.h"

#include <string>

#include "app_state.h"

#include "esp_event.h"
#include "esp_timer.h"

namespace powerrelay {

std::string mqtt_base_topic() {
    std::string topic = trim_copy(gConfig.mqttBaseTopic);
    while (!topic.empty() && topic.back() == '/') {
        topic.pop_back();
    }
    if (topic.empty()) {
        topic = "power_relay";
    }
    return topic;
}

std::string mqtt_topic_set() {
    return mqtt_base_topic() + "/set";
}

std::string mqtt_topic_availability() {
    return mqtt_base_topic() + "/availability";
}

void mqtt_disconnect() {
    if (gMqttClient == nullptr) {
        return;
    }

    esp_mqtt_client_stop(gMqttClient);
    esp_mqtt_client_destroy(gMqttClient);
    gMqttClient = nullptr;
}

void mqtt_publish_state(bool force) {
    static bool lastState = false;

    if (gMqttClient == nullptr || gConfig.mqttEnabled == 0) {
        return;
    }
    if (!force && gRelayStateOn == lastState) {
        return;
    }

    std::string topic = mqtt_base_topic() + "/state";
    esp_mqtt_client_publish(gMqttClient, topic.c_str(), gRelayStateOn ? "ON" : "OFF", 0, 1, 1);
    lastState = gRelayStateOn;
}

void mqtt_event_handler(void*, esp_event_base_t, int32_t event_id, void* event_data) {
    auto* event = static_cast<esp_mqtt_event_handle_t>(event_data);

    if (event_id == MQTT_EVENT_CONNECTED) {
        std::string commandTopic = mqtt_base_topic() + "/set";
        std::string availTopic = mqtt_base_topic() + "/availability";
        esp_mqtt_client_subscribe(gMqttClient, commandTopic.c_str(), 1);
        esp_mqtt_client_publish(gMqttClient, availTopic.c_str(), "online", 0, 1, 1);
        mqtt_publish_state(true);
        return;
    }

    if (event_id == MQTT_EVENT_DATA) {
        std::string topic(event->topic, event->topic_len);
        std::string data(event->data, event->data_len);
        if (topic == mqtt_topic_set()) {
            data = to_lowercase(data);
            if (data == "toggle" || data == "on" || data == "power") {
                toggle_relay();
                mqtt_publish_state(true);
            }
        }
    }
}

void mqtt_ensure_connected() {
    if (gConfig.mqttEnabled == 0 || gAccessPointMode || !gStaConnected) {
        mqtt_disconnect();
        return;
    }

    if (gMqttClient != nullptr) {
        return;
    }

    int64_t now = esp_timer_get_time();
    if (now - gLastMqttAttemptUs < MQTT_RECONNECT_US) {
        return;
    }
    gLastMqttAttemptUs = now;

    std::string host = trim_copy(gConfig.mqttHost);
    if (host.empty()) {
        return;
    }

    std::string uri = "mqtt://" + host + ":" + std::to_string(gConfig.mqttPort == 0 ? 1883 : gConfig.mqttPort);
    std::string topic = mqtt_topic_availability();
    std::string user = trim_copy(gConfig.mqttUser);
    std::string pass = trim_copy(gConfig.mqttPassword);

    esp_mqtt_client_config_t cfg = {};
    cfg.broker.address.uri = uri.c_str();
    cfg.session.last_will.topic = topic.c_str();
    cfg.session.last_will.msg = "offline";
    cfg.session.last_will.qos = 1;
    cfg.session.last_will.retain = 1;

    if (!user.empty()) {
        cfg.credentials.username = user.c_str();
        cfg.credentials.authentication.password = pass.c_str();
    }

    gMqttClient = esp_mqtt_client_init(&cfg);
    if (gMqttClient == nullptr) {
        return;
    }

    esp_mqtt_client_register_event(gMqttClient, MQTT_EVENT_ANY, mqtt_event_handler, nullptr);
    if (esp_mqtt_client_start(gMqttClient) != ESP_OK) {
        mqtt_disconnect();
    }
}

}  // namespace powerrelay

