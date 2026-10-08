#include "wifi.h"

#include <cstring>
#include <string>

#include "app_state.h"

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi.h"

namespace powerrelay {

void stop_wifi() {
    esp_wifi_stop();
    esp_wifi_set_mode(WIFI_MODE_NULL);
}

void start_access_point_mode() {
    stop_wifi();

    wifi_config_t ap_cfg = {};
    std::string ap_name = sanitize_hostname(gConfig.hostName);
    safe_copy(reinterpret_cast<char*>(ap_cfg.ap.ssid), sizeof(ap_cfg.ap.ssid), ap_name);
    ap_cfg.ap.ssid_len = std::strlen(reinterpret_cast<const char*>(ap_cfg.ap.ssid));
    ap_cfg.ap.channel = 1;
    ap_cfg.ap.max_connection = 4;
    ap_cfg.ap.authmode = WIFI_AUTH_OPEN;

    esp_netif_ip_info_t ip_info = {};
    esp_netif_str_to_ip4(AP_FALLBACK_IP, &ip_info.ip);
    esp_netif_str_to_ip4(AP_FALLBACK_GW, &ip_info.gw);
    esp_netif_str_to_ip4(AP_FALLBACK_MASK, &ip_info.netmask);

    esp_netif_t* ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (ap_netif != nullptr) {
        esp_netif_dhcps_stop(ap_netif);
        esp_netif_set_ip_info(ap_netif, &ip_info);
        esp_netif_dhcps_start(ap_netif);
    }

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &ap_cfg);
    esp_wifi_start();

    gAccessPointMode = true;
    gStaConnected = false;
}

void start_station_mode() {
    stop_wifi();

    wifi_config_t sta_cfg = {};
    safe_copy(reinterpret_cast<char*>(sta_cfg.sta.ssid), sizeof(sta_cfg.sta.ssid), gConfig.wifiSsid);
    safe_copy(reinterpret_cast<char*>(sta_cfg.sta.password), sizeof(sta_cfg.sta.password), gConfig.wifiPassword);

    std::string host = sanitize_hostname(gConfig.hostName);
    esp_netif_t* sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (sta_netif != nullptr) {
        esp_netif_set_hostname(sta_netif, host.c_str());
    }

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &sta_cfg);
    esp_wifi_start();

    gAccessPointMode = false;
}

void apply_wifi_mode() {
    if (std::strlen(gConfig.wifiSsid) == 0 && std::strlen(gConfig.wifiPassword) == 0) {
        start_access_point_mode();
    } else {
        start_station_mode();
    }
}

void wifi_event_handler(void*, esp_event_base_t event_base, int32_t event_id, void*) {
    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_START) {
            esp_wifi_connect();
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            gStaConnected = false;
        }
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        gStaConnected = true;
    }
}

void maybe_retry_wifi() {
    if (gAccessPointMode || gStaConnected) {
        return;
    }

    int64_t now = esp_timer_get_time();
    if (now - gLastWifiRetryCheckUs < WIFI_RETRY_CHECK_US) {
        return;
    }
    gLastWifiRetryCheckUs = now;

    static int64_t lastRetryUs = 0;
    if (now - lastRetryUs >= static_cast<int64_t>(gConfig.wifiReconnectIntervalMs) * 1000) {
        lastRetryUs = now;
        esp_wifi_connect();
    }
}

}  // namespace powerrelay


