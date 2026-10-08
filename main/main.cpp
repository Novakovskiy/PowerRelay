#include "app_state.h"
#include "hardware.h"
#include "mqtt.h"
#include "uart.h"
#include "web.h"
#include "wifi.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

namespace powerrelay {

[[noreturn]] void firmware_task(void*) {
    while (true) {
        process_boot_button();
        update_status_led();
        maybe_retry_wifi();
        mqtt_ensure_connected();
        mqtt_publish_state(false);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

}  // namespace powerrelay

extern "C" void app_main() {
    using namespace powerrelay;

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_flash_init failed: %s", esp_err_to_name(err));
        return;
    }

    // Keep boot/runtime UART output compact unless a warning/error happens.
    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(TAG, ESP_LOG_INFO);

    load_config_nvs();
    configure_gpio();

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t wifiCfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifiCfg);

    esp_event_handler_instance_t wifiHandler = nullptr;
    esp_event_handler_instance_t ipHandler = nullptr;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr, &wifiHandler);
    esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr, &ipHandler);

    apply_wifi_mode();
    start_http_server();

    xTaskCreate(&firmware_task, "firmware_task", 8192, nullptr, 5, nullptr);
}
