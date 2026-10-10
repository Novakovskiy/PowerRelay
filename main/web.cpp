#include "web.h"

#include <cstring>
#include <string>

#include "api.h"
#include "app_state.h"
#include "mqtt.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_http_server.h"
#include "esp_system.h"

#include "favicon_data.h"
#include "web_page.h"

namespace powerrelay {

esp_err_t handle_index(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    const char* page = gAccessPointMode ? SETTINGS_HTML : INDEX_HTML;
    return httpd_resp_send(req, page, HTTPD_RESP_USE_STRLEN);
}

esp_err_t handle_settings_page(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_send(req, SETTINGS_HTML, HTTPD_RESP_USE_STRLEN);
}

esp_err_t handle_favicon(httpd_req_t* req) {
    httpd_resp_set_type(req, "image/x-icon");
    return httpd_resp_send(req, reinterpret_cast<const char*>(FAVICON_ICO), sizeof(FAVICON_ICO));
}

esp_err_t handle_status(httpd_req_t* req) {
    std::string json = R"({"powerOn":)";
    json += (gRelayStateOn ? "true" : "false");
    json += R"(,"pendingOff":)";
    json += (is_relay_off_pending() ? "true" : "false");
    json += R"(,"pendingOffMs":)";
    json += std::to_string(relay_off_pending_ms());
    json += "}";
    return send_json(req, json);
}

esp_err_t handle_get_settings(httpd_req_t* req) {
    return send_json(req, json_from_config());
}

esp_err_t handle_post_settings(httpd_req_t* req) {
    std::string body;
    body.resize(static_cast<size_t>(req->content_len));

    int got = httpd_req_recv(req, body.data(), req->content_len);
    if (got < 0) {
        return httpd_resp_send_500(req);
    }

    body.resize(static_cast<size_t>(got));
    apply_config_from_form(body);

    if (gConfig.mqttEnabled == 0) {
        mqtt_disconnect();
    }

    return send_json(req, R"({"ok":true})");
}

esp_err_t handle_action(httpd_req_t* req) {
    char query[128] = {0};
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_set_type(req, "text/plain");
        return httpd_resp_send(req, "Missing cmd", HTTPD_RESP_USE_STRLEN);
    }

    char cmd[32] = {0};
    if (httpd_query_key_value(query, "cmd", cmd, sizeof(cmd)) != ESP_OK) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_set_type(req, "text/plain");
        return httpd_resp_send(req, "Missing cmd", HTTPD_RESP_USE_STRLEN);
    }

    std::string command = cmd;
    for (char& c : command) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }

    if (command == "on") {
        toggle_relay();
        mqtt_publish_state(true);
        httpd_resp_set_type(req, "text/plain");
        return httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    }

    httpd_resp_set_status(req, "400 Bad Request");
    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_send(req, "Unknown cmd", HTTPD_RESP_USE_STRLEN);
}

esp_err_t handle_reboot(httpd_req_t* req) {
    send_json(req, R"({"ok":true,"message":"rebooting"})");
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
    __builtin_unreachable();
}

esp_err_t handle_reset(httpd_req_t* req) {
    init_default_config(gConfig);
    save_config_nvs();
    set_relay(false);
    mqtt_disconnect();
    send_json(req, R"({"ok":true,"message":"resetting"})");
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
    __builtin_unreachable();
}

esp_err_t handle_not_found(httpd_req_t* req, httpd_err_code_t) {
    if (gAccessPointMode) {
        return handle_settings_page(req);
    }
    return httpd_resp_send_404(req);
}

constexpr httpd_uri_t root = {"/", HTTP_GET, handle_index, nullptr};
constexpr httpd_uri_t settings = {"/settings", HTTP_GET, handle_settings_page, nullptr};
constexpr httpd_uri_t favicon = {"/favicon.ico", HTTP_GET, handle_favicon, nullptr};
constexpr httpd_uri_t status = {"/status", HTTP_GET, handle_status, nullptr};
constexpr httpd_uri_t get_settings = {"/api/settings", HTTP_GET, handle_get_settings, nullptr};
constexpr httpd_uri_t post_settings = {"/api/settings", HTTP_POST, handle_post_settings, nullptr};
constexpr httpd_uri_t action = {"/action", HTTP_POST, handle_action, nullptr};
constexpr httpd_uri_t reboot = {"/api/reboot", HTTP_POST, handle_reboot, nullptr};
constexpr httpd_uri_t reset = {"/api/reset", HTTP_POST, handle_reset, nullptr};

void start_http_server() {
    if (gHttpServer != nullptr) {
        return;
    }

    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.max_uri_handlers = 16;

    if (httpd_start(&gHttpServer, &cfg) != ESP_OK) {
        return;
    }

    httpd_register_uri_handler(gHttpServer, &root);
    httpd_register_uri_handler(gHttpServer, &settings);
    httpd_register_uri_handler(gHttpServer, &favicon);
    httpd_register_uri_handler(gHttpServer, &status);
    httpd_register_uri_handler(gHttpServer, &get_settings);
    httpd_register_uri_handler(gHttpServer, &post_settings);
    httpd_register_uri_handler(gHttpServer, &action);
    httpd_register_uri_handler(gHttpServer, &reboot);
    httpd_register_uri_handler(gHttpServer, &reset);
    httpd_register_err_handler(gHttpServer, HTTPD_404_NOT_FOUND, handle_not_found);
}

}  // namespace powerrelay
