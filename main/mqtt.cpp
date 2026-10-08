#include "mqtt.h"

#include <string>

#include "app_state.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_timer.h"

namespace powerrelay {

std::string mqtt_base_topic();

namespace {

const char* connack_to_text(int code) {
    switch (code) {
        case MQTT_CONNECTION_ACCEPTED:
            return "accepted";
        case MQTT_CONNECTION_REFUSE_PROTOCOL:
            return "protocol";
        case MQTT_CONNECTION_REFUSE_ID_REJECTED:
            return "id_rejected";
        case MQTT_CONNECTION_REFUSE_SERVER_UNAVAILABLE:
            return "server_unavailable";
        case MQTT_CONNECTION_REFUSE_BAD_USERNAME:
            return "bad_username_or_password";
        case MQTT_CONNECTION_REFUSE_NOT_AUTHORIZED:
            return "not_authorized";
        default:
            return "unknown";
    }
}

std::string mqtt_entity_id() {
    std::string source = mqtt_base_topic();
    if (source.empty()) {
        source = "power_relay";
    }

    std::string out;
    out.reserve(source.size());

    bool prevUnderscore = false;
    for (char c : source) {
        const bool isAlnum = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
        if (isAlnum) {
            char lowered = c;
            if (lowered >= 'A' && lowered <= 'Z') {
                lowered = static_cast<char>(lowered - 'A' + 'a');
            }
            out.push_back(lowered);
            prevUnderscore = false;
        } else if (!prevUnderscore) {
            out.push_back('_');
            prevUnderscore = true;
        }
    }

    while (!out.empty() && out.front() == '_') {
        out.erase(out.begin());
    }
    while (!out.empty() && out.back() == '_') {
        out.pop_back();
    }

    if (out.empty()) {
        out = "power_relay";
    }
    return out;
}

void mqtt_publish_discovery() {
    if (gMqttClient == nullptr || gConfig.mqttHaDiscovery == 0) {
        return;
    }

    const std::string entityId = mqtt_entity_id();
    const std::string baseTopic = mqtt_base_topic();
    const std::string discoveryTopic = "homeassistant/switch/" + entityId + "/config";
    const std::string availabilityTopic = baseTopic + "/availability";
    const std::string stateTopic = baseTopic + "/state";
    const std::string commandTopic = baseTopic + "/set";
    const std::string deviceName = trim_copy(gConfig.hostName).empty() ? "Power Relay" : std::string(gConfig.hostName);

    std::string payload = "{";
    payload += R"("name":")" + deviceName + R"(",)";
    payload += R"("uniq_id":")" + entityId + R"(",)";
    payload += R"("cmd_t":")" + commandTopic + R"(",)";
    payload += R"("stat_t":")" + stateTopic + R"(",)";
    payload += R"("avty_t":")" + availabilityTopic + R"(",)";
    payload += R"("pl_on":"ON","pl_off":"OFF",)";
    payload += R"("stat_on":"ON","stat_off":"OFF",)";
    payload += R"("icon":"mdi:power-socket-eu",)";
    payload += R"("device":{"ids":[")" + entityId + R"("],"name":")" + deviceName + R"(","mdl":"Power Relay","mf":"PowerRelay"})";
    payload += "}";

    ESP_LOGW(TAG, "Publishing Home Assistant discovery to %s", discoveryTopic.c_str());
    esp_mqtt_client_publish(gMqttClient, discoveryTopic.c_str(), payload.c_str(), 0, 1, 1);
}

}  // namespace

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
        ESP_LOGW(TAG, "MQTT connected");
        std::string commandTopic = mqtt_base_topic() + "/set";
        std::string availTopic = mqtt_base_topic() + "/availability";
        esp_mqtt_client_subscribe(gMqttClient, commandTopic.c_str(), 1);
        esp_mqtt_client_publish(gMqttClient, availTopic.c_str(), "online", 0, 1, 1);
        mqtt_publish_discovery();
        mqtt_publish_state(true);
        return;
    }

    if (event_id == MQTT_EVENT_DISCONNECTED) {
        ESP_LOGW(TAG, "MQTT disconnected");
        return;
    }

    if (event_id == MQTT_EVENT_ERROR) {
        if (event->error_handle == nullptr) {
            ESP_LOGE(TAG, "MQTT error: missing error details");
            return;
        }

        const int errorType = event->error_handle->error_type;
        const int connack = event->error_handle->connect_return_code;
        ESP_LOGE(TAG, "MQTT error: type=%d, connect_return_code=%d (%s), esp_tls_last_esp_err=0x%x, esp_tls_stack_err=0x%x, esp_transport_sock_errno=%d",
                 errorType, connack, connack_to_text(connack), static_cast<unsigned>(event->error_handle->esp_tls_last_esp_err),
                 static_cast<unsigned>(event->error_handle->esp_tls_stack_err), event->error_handle->esp_transport_sock_errno);
        return;
    }

    if (event_id == MQTT_EVENT_DATA) {
        std::string topic(event->topic, event->topic_len);
        std::string data(event->data, event->data_len);
        if (topic == mqtt_topic_set()) {
            data = to_lowercase(data);
            if (data == "on" || data == "1" || data == "true") {
                set_relay(true);
                mqtt_publish_state(true);
            } else if (data == "off" || data == "0" || data == "false") {
                set_relay(false);
                mqtt_publish_state(true);
            } else if (data == "toggle" || data == "power") {
                toggle_relay();
                mqtt_publish_state(true);
            } else {
                ESP_LOGW(TAG, "Unsupported MQTT command payload: '%s'", data.c_str());
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
    std::string user(gConfig.mqttUser);
    std::string pass(gConfig.mqttPassword);

    esp_mqtt_client_config_t cfg = {};
    cfg.broker.address.uri = uri.c_str();
    cfg.session.last_will.topic = topic.c_str();
    cfg.session.last_will.msg = "offline";
    cfg.session.last_will.qos = 1;
    cfg.session.last_will.retain = 1;

    if (!user.empty()) {
        cfg.credentials.username = user.c_str();
    }
    if (!pass.empty()) {
        cfg.credentials.authentication.password = pass.c_str();
    }

    ESP_LOGW(TAG, "MQTT connecting to %s (username set: %s, password set: %s)", uri.c_str(), user.empty() ? "no" : "yes",
             pass.empty() ? "no" : "yes");

    gMqttClient = esp_mqtt_client_init(&cfg);
    if (gMqttClient == nullptr) {
        ESP_LOGE(TAG, "MQTT init failed");
        return;
    }

    esp_mqtt_client_register_event(gMqttClient, MQTT_EVENT_ANY, mqtt_event_handler, nullptr);
    if (esp_mqtt_client_start(gMqttClient) != ESP_OK) {
        ESP_LOGE(TAG, "MQTT start failed");
        mqtt_disconnect();
    }
}

}  // namespace powerrelay
