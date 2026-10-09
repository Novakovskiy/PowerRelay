#include "uart.h"

#include <cstring>
#include <string>

#include "app_state.h"

#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"

namespace powerrelay {

namespace {

constexpr uart_port_t UART_CONTROL_PORT = UART_NUM_0;
constexpr int UART_RX_BUFFER_SIZE = 256;
constexpr int UART_TX_BUFFER_SIZE = 0;
constexpr int UART_READ_CHUNK_SIZE = 64;
constexpr size_t UART_LINE_BUFFER_MAX = 256;

bool gUartControlReady = false;
std::string gUartRxBuffer;

std::string trim_ascii(std::string value) {
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t' || value.front() == '\r' || value.front() == '\n')) {
        value.erase(value.begin());
    }
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || value.back() == '\r' || value.back() == '\n')) {
        value.pop_back();
    }
    return value;
}

void process_uart_line(const std::string& line) {
    std::string cmd = to_lowercase(trim_ascii(line));
    if (cmd.empty()) {
        return;
    }

    if (cmd == "power_on") {
        set_relay_from_uart(true);
        return;
    }
    if (cmd == "power_off") {
        set_relay_from_uart(false);
        return;
    }

    ESP_LOGW(TAG, "Unsupported UART command: '%s'", cmd.c_str());
}

}  // namespace

void uart_control_init() {
    if (gUartControlReady) {
        return;
    }

    esp_err_t err = uart_driver_install(UART_CONTROL_PORT, UART_RX_BUFFER_SIZE, UART_TX_BUFFER_SIZE, 0, nullptr, 0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "UART driver install failed: %s", esp_err_to_name(err));
        return;
    }

    gUartControlReady = true;
}

void uart_control_send_power_state(bool on) {
    if (!gConfig.uartControl) {
        return;
    }

    uart_control_init();
    if (!gUartControlReady) {
        return;
    }

    const char* payload = on ? "power_on\n" : "power_off\n";
    int written = uart_write_bytes(UART_CONTROL_PORT, payload, std::strlen(payload));
    if (written < 0) {
        ESP_LOGE(TAG, "UART write failed");
    }
}

void uart_control_poll() {
    if (!gConfig.uartControl) {
        gUartRxBuffer.clear();
        return;
    }

    uart_control_init();
    if (!gUartControlReady) {
        return;
    }

    uint8_t chunk[UART_READ_CHUNK_SIZE];
    int read = uart_read_bytes(UART_CONTROL_PORT, chunk, sizeof(chunk), 0);
    if (read <= 0) {
        return;
    }

    gUartRxBuffer.append(reinterpret_cast<const char*>(chunk), static_cast<size_t>(read));

    size_t lineEndPos = gUartRxBuffer.find_first_of("\r\n");
    while (lineEndPos != std::string::npos) {
        std::string line = gUartRxBuffer.substr(0, lineEndPos);
        process_uart_line(line);

        size_t eraseUntil = lineEndPos;
        while (eraseUntil < gUartRxBuffer.size() && (gUartRxBuffer[eraseUntil] == '\r' || gUartRxBuffer[eraseUntil] == '\n')) {
            ++eraseUntil;
        }
        gUartRxBuffer.erase(0, eraseUntil);
        lineEndPos = gUartRxBuffer.find_first_of("\r\n");
    }

    if (gUartRxBuffer.size() > UART_LINE_BUFFER_MAX) {
        ESP_LOGW(TAG, "UART input buffer overflow, dropping stale data");
        gUartRxBuffer.clear();
    }
}

}  // namespace powerrelay
