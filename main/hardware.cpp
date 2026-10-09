#include "hardware.h"

#include "app_state.h"
#include "mqtt.h"

#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_timer.h"

namespace powerrelay {

void configure_gpio() {
    gpio_config_t output_cfg = {};
    output_cfg.intr_type = GPIO_INTR_DISABLE;
    output_cfg.mode = GPIO_MODE_OUTPUT;
    output_cfg.pin_bit_mask = (1ULL << PIN_POWER_SW) | (1ULL << PIN_STATUS_LED);
    output_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    output_cfg.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&output_cfg);

    gpio_config_t input_cfg = {};
    input_cfg.intr_type = GPIO_INTR_DISABLE;
    input_cfg.mode = GPIO_MODE_INPUT;
    input_cfg.pin_bit_mask = (1ULL << PIN_BOOT_BUTTON);
    input_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    input_cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&input_cfg);

    gRelayStateOn = false;
    gpio_set_level(PIN_POWER_SW, 0);
    gpio_set_level(PIN_STATUS_LED, 0);
}

void update_status_led() {
    if (gAccessPointMode) {
        gStatusLedStateOn = true;
        gpio_set_level(PIN_STATUS_LED, 1);
        return;
    }

    if (!gStaConnected) {
        gStatusLedStateOn = false;
        gpio_set_level(PIN_STATUS_LED, 0);
        return;
    }

    int64_t now = esp_timer_get_time() / 1000;
    if (now - gLastStatusToggleMs >= STATUS_LED_BLINK_MS) {
        gLastStatusToggleMs = now;
        gStatusLedStateOn = !gStatusLedStateOn;
        gpio_set_level(PIN_STATUS_LED, gStatusLedStateOn ? 1 : 0);
    }
}

void process_boot_button() {
    bool pressed = gpio_get_level(PIN_BOOT_BUTTON) == 0;
    int64_t now = esp_timer_get_time() / 1000;

    if (pressed) {
        if (gBootPressedSinceMs == 0) {
            gBootPressedSinceMs = now;
        }

        if (!gBootResetTriggered && now - gBootPressedSinceMs >= BUTTON_RESET_MS) {
            gBootResetTriggered = true;
            init_default_config(gConfig);
            save_config_nvs();
            mqtt_disconnect();
            esp_restart();
        }
    } else {
        gBootPressedSinceMs = 0;
        gBootResetTriggered = false;
    }
}

}  // namespace powerrelay
