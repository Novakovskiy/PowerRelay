#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "driver/gpio.h"
#include "esp_http_server.h"
#include "mqtt_client.h"

namespace powerrelay {

inline constexpr char TAG[] = "power_relay";

inline constexpr gpio_num_t PIN_POWER_SW = GPIO_NUM_2;
inline constexpr gpio_num_t PIN_STATUS_LED = GPIO_NUM_4;
inline constexpr gpio_num_t PIN_BOOT_BUTTON = GPIO_NUM_0;

inline constexpr int64_t STATUS_LED_BLINK_US = 500000;
inline constexpr int64_t BUTTON_RESET_US = 10000000;
inline constexpr int64_t MQTT_RECONNECT_US = 10000000;
inline constexpr int64_t WIFI_RETRY_CHECK_US = 500000;

inline constexpr uint32_t WIFI_RECONNECT_MIN_MS = 10000;
inline constexpr uint32_t UART_TIMEOUT_MIN_MS = 10000;
inline constexpr uint32_t UART_TIMEOUT_MAX_MS = 600000;

inline constexpr char NVS_NAMESPACE[] = "config";
inline constexpr char AP_FALLBACK_IP[] = "192.168.4.1";
inline constexpr char AP_FALLBACK_GW[] = "192.168.4.1";
inline constexpr char AP_FALLBACK_MASK[] = "255.255.255.0";

struct PersistentConfig {
    uint32_t wifiReconnectIntervalMs;
    char wifiSsid[24];
    char wifiPassword[24];
    char hostName[24];
    uint8_t uartControl;
    uint32_t uartTimeout;
    uint8_t mqttEnabled;
    uint16_t mqttPort;
    uint8_t mqttHaDiscovery;
    char mqttHost[64];
    char mqttUser[24];
    char mqttPassword[24];
    char mqttBaseTopic[48];
};

extern PersistentConfig gConfig;
extern httpd_handle_t gHttpServer;
extern esp_mqtt_client_handle_t gMqttClient;

extern bool gRelayStateOn;
extern bool gStatusLedStateOn;
extern bool gStaConnected;
extern bool gAccessPointMode;

extern int64_t gLastStatusToggleUs;
extern int64_t gLastWifiRetryCheckUs;
extern int64_t gLastMqttAttemptUs;
extern int64_t gBootPressedSinceUs;
extern bool gBootResetTriggered;

void safe_copy(char* dst, size_t dstSize, const std::string& src);
std::string trim_copy(const char* value);
std::string sanitize_hostname(const char* raw);
std::string to_lowercase(std::string str);

void init_default_config(PersistentConfig& cfg);
void sanitize_config(PersistentConfig& cfg);
void save_config_nvs();
void load_config_nvs();

void set_relay(bool on);
void toggle_relay();

}  // namespace powerrelay


