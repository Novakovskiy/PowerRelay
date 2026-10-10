#include "app_state.h"

#include <cstdlib>
#include <cstring>

#include "uart.h"

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "nvs.h"

namespace powerrelay {

PersistentConfig gConfig{};
httpd_handle_t gHttpServer = nullptr;
esp_mqtt_client_handle_t gMqttClient = nullptr;

bool gRelayStateOn = false;
bool gStatusLedStateOn = false;
bool gStaConnected = false;
bool gAccessPointMode = false;

int64_t gLastStatusToggleMs = 0;
int64_t gLastWifiRetryCheckMs = 0;
int64_t gLastMqttAttemptMs = 0;
int64_t gBootPressedSinceMs = 0;
bool gBootResetTriggered = false;

namespace {

bool gPendingRelayOff = false;
int64_t gPendingRelayOffAtMs = 0;

int64_t now_ms() {
    return esp_timer_get_time() / 1000;
}

void apply_relay_output(bool on) {
    gRelayStateOn = on;
    gpio_set_level(PIN_POWER_SW, gRelayStateOn ? 1 : 0);
}

void schedule_relay_off() {
    uint32_t timeoutMs = gConfig.uartTimeout;
    if (timeoutMs < UART_TIMEOUT_MIN_MS || timeoutMs > UART_TIMEOUT_MAX_MS) {
        timeoutMs = 30000;
    }
    gPendingRelayOff = true;
    gPendingRelayOffAtMs = now_ms() + static_cast<int64_t>(timeoutMs);
}

void set_relay_internal(bool on, bool emitUartMessage) {
    if (on) {
        gPendingRelayOff = false;
        gPendingRelayOffAtMs = 0;
        if (emitUartMessage && gConfig.uartControl) {
            uart_control_send_power_state(true);
        }
        apply_relay_output(true);
        return;
    }

    if (gConfig.uartControl) {
        if (emitUartMessage) {
            uart_control_send_power_state(false);
        }
        schedule_relay_off();
        return;
    }

    gPendingRelayOff = false;
    gPendingRelayOffAtMs = 0;
    apply_relay_output(false);
}

}  // namespace

void safe_copy(char* dst, size_t dstSize, const std::string& src) {
    if (dstSize == 0) {
        return;
    }

    std::strncpy(dst, src.c_str(), dstSize - 1);
    dst[dstSize - 1] = '\0';
}

std::string trim_copy(const char* value) {
    if (value == nullptr) {
        return "";
    }

    std::string out(value);
    while (!out.empty() && (out.front() == ' ' || out.front() == '\t' || out.front() == '\n' || out.front() == '\r')) {
        out.erase(out.begin());
    }
    while (!out.empty() && (out.back() == ' ' || out.back() == '\t' || out.back() == '\n' || out.back() == '\r')) {
        out.pop_back();
    }
    return out;
}

std::string sanitize_hostname(const char* raw) {
    std::string input = trim_copy(raw);
    std::string out;

    for (char c : input) {
        bool isAlphaNum =
            (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9');
        if (isAlphaNum || c == '-') {
            out.push_back(c);
        }
    }

    while (!out.empty() && out.front() == '-') {
        out.erase(out.begin());
    }
    while (!out.empty() && out.back() == '-') {
        out.pop_back();
    }

    if (out.empty()) {
        out = "PowerRelay";
    }

    if (out.size() > 32) {
        out.resize(32);
    }
    return out;
}

std::string to_lowercase(std::string str) {
    for (char& c : str) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    return str;
}

void init_default_config(PersistentConfig& cfg) {
    std::memset(&cfg, 0, sizeof(cfg));
    cfg.wifiReconnectIntervalMs = 30000;
    std::strncpy(cfg.wifiSsid, "", sizeof(cfg.wifiSsid) - 1);
    std::strncpy(cfg.wifiPassword, "", sizeof(cfg.wifiPassword) - 1);
    std::strncpy(cfg.hostName, "Power Relay", sizeof(cfg.hostName) - 1);
    cfg.uartControl = 0;
    cfg.uartTimeout = 30000;
    cfg.mqttEnabled = 0;
    cfg.mqttPort = 1883;
    cfg.mqttHaDiscovery = 0;
    std::strncpy(cfg.mqttBaseTopic, "power_relay", sizeof(cfg.mqttBaseTopic) - 1);
}

void sanitize_config(PersistentConfig& cfg) {
    cfg.wifiSsid[sizeof(cfg.wifiSsid) - 1] = '\0';
    cfg.wifiPassword[sizeof(cfg.wifiPassword) - 1] = '\0';
    cfg.hostName[sizeof(cfg.hostName) - 1] = '\0';
    cfg.mqttHost[sizeof(cfg.mqttHost) - 1] = '\0';
    cfg.mqttUser[sizeof(cfg.mqttUser) - 1] = '\0';
    cfg.mqttPassword[sizeof(cfg.mqttPassword) - 1] = '\0';
    cfg.mqttBaseTopic[sizeof(cfg.mqttBaseTopic) - 1] = '\0';

    if (cfg.wifiReconnectIntervalMs < WIFI_RECONNECT_MIN_MS) {
        cfg.wifiReconnectIntervalMs = WIFI_RECONNECT_MIN_MS;
    }
    if (cfg.uartControl > 1) {
        cfg.uartControl = 0;
    }
    if (cfg.uartTimeout < UART_TIMEOUT_MIN_MS || cfg.uartTimeout > UART_TIMEOUT_MAX_MS) {
        cfg.uartTimeout = 30000;
    }
    if (cfg.mqttPort == 0) {
        cfg.mqttPort = 1883;
    }
    if (std::strlen(cfg.hostName) == 0) {
        std::strncpy(cfg.hostName, "Power Relay", sizeof(cfg.hostName) - 1);
        cfg.hostName[sizeof(cfg.hostName) - 1] = '\0';
    }
}

void save_config_nvs() {
    sanitize_config(gConfig);

    nvs_handle_t handle = 0;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) {
        return;
    }

    esp_err_t err = nvs_set_blob(handle, "cfg", &gConfig, sizeof(gConfig));
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
}

void load_config_nvs() {
    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        init_default_config(gConfig);
        return;
    }

    size_t blob_size = sizeof(gConfig);
    err = nvs_get_blob(handle, "cfg", &gConfig, &blob_size);

    if (err != ESP_OK || blob_size != sizeof(gConfig)) {
        init_default_config(gConfig);
        nvs_set_blob(handle, "cfg", &gConfig, sizeof(gConfig));
        nvs_commit(handle);
    }

    nvs_close(handle);
    sanitize_config(gConfig);
}

void set_relay(bool on) {
    set_relay_internal(on, true);
}

void set_relay_from_uart(bool on) {
    set_relay_internal(on, false);
}

void toggle_relay() {
    set_relay(!gRelayStateOn);
}

void process_relay_timers() {
    if (!gPendingRelayOff) {
        return;
    }

    int64_t now = now_ms();
    if (now < gPendingRelayOffAtMs) {
        return;
    }

    gPendingRelayOff = false;
    gPendingRelayOffAtMs = 0;
    apply_relay_output(false);
}

bool is_relay_off_pending() {
    return gPendingRelayOff;
}

uint32_t relay_off_pending_ms() {
    if (!gPendingRelayOff) {
        return 0;
    }

    int64_t now = now_ms();
    if (now >= gPendingRelayOffAtMs) {
        return 0;
    }

    int64_t remainingMs = gPendingRelayOffAtMs - now;
    return static_cast<uint32_t>(remainingMs);
}

}  // namespace powerrelay
