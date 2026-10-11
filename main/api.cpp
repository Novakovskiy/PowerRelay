#include "api.h"

#include <cstdlib>

#include "app_state.h"
#include "mqtt.h"

namespace powerrelay {

std::string json_escape(const char* value) {
    std::string in(value);
    std::string out;
    out.reserve(in.size() + 8);

    for (char c : in) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out.push_back(c); break;
        }
    }

    return out;
}

std::string json_from_config() {
    std::string out = "{";
    out += R"("wifiReconnectMs":)" + std::to_string(gConfig.wifiReconnectIntervalMs);
    out += R"(,"wifiSsid":")" + json_escape(gConfig.wifiSsid) + R"(")";
    out += R"(,"wifiPassword":")" + json_escape(gConfig.wifiPassword) + R"(")";
    out += R"(,"hostName":")" + json_escape(gConfig.hostName) + R"(")";
    out += R"(,"powerOnByDefault":)" + std::string(gConfig.powerOnByDefault ? "true" : "false");
    out += R"(,"uartControl":)" + std::string(gConfig.uartControl ? "true" : "false");
    out += R"(,"uartTimeout":)" + std::to_string(gConfig.uartTimeout);
    out += R"(,"mqttEnabled":)" + std::string(gConfig.mqttEnabled ? "true" : "false");
    out += R"(,"mqttPort":)" + std::to_string(gConfig.mqttPort);
    out += R"(,"mqttHaDiscovery":)" + std::string(gConfig.mqttHaDiscovery ? "true" : "false");
    out += R"(,"mqttHost":")" + json_escape(gConfig.mqttHost) + R"(")";
    out += R"(,"mqttUser":")" + json_escape(gConfig.mqttUser) + R"(")";
    out += R"(,"mqttPassword":")" + json_escape(gConfig.mqttPassword) + R"(")";
    out += R"(,"mqttBaseTopic":")" + json_escape(gConfig.mqttBaseTopic) + R"(")";
    out += R"(,"maxLens":{)";
    out += R"("wifiSsid":)" + std::to_string(sizeof(gConfig.wifiSsid) - 1);
    out += R"(,"wifiPassword":)" + std::to_string(sizeof(gConfig.wifiPassword) - 1);
    out += R"(,"hostName":)" + std::to_string(sizeof(gConfig.hostName) - 1);
    out += R"(,"mqttHost":)" + std::to_string(sizeof(gConfig.mqttHost) - 1);
    out += R"(,"mqttUser":)" + std::to_string(sizeof(gConfig.mqttUser) - 1);
    out += R"(,"mqttPassword":)" + std::to_string(sizeof(gConfig.mqttPassword) - 1);
    out += R"(,"mqttBaseTopic":)" + std::to_string(sizeof(gConfig.mqttBaseTopic) - 1);
    out += R"(}})";
    return out;
}

std::string url_decode(const std::string& src) {
    std::string out;
    out.reserve(src.size());

    for (size_t i = 0; i < src.size(); ++i) {
        if (src[i] == '+') {
            out.push_back(' ');
            continue;
        }
        if (src[i] == '%' && i + 2 < src.size()) {
            auto hex_val = [](char c) -> int {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                return -1;
            };
            int hi = hex_val(src[i + 1]);
            int lo = hex_val(src[i + 2]);
            if (hi >= 0 && lo >= 0) {
                out.push_back(static_cast<char>((hi << 4) | lo));
                i += 2;
                continue;
            }
        }
        out.push_back(src[i]);
    }

    return out;
}

std::string get_form_value(const std::string& body, const std::string& key) {
    size_t pos = 0;
    while (pos < body.size()) {
        size_t amp = body.find('&', pos);
        size_t end = (amp == std::string::npos) ? body.size() : amp;
        std::string pair = body.substr(pos, end - pos);

        size_t eq = pair.find('=');
        std::string name = eq == std::string::npos ? pair : pair.substr(0, eq);
        std::string value = eq == std::string::npos ? "" : pair.substr(eq + 1);

        if (name == key) {
            return url_decode(value);
        }

        if (amp == std::string::npos) {
            break;
        }
        pos = amp + 1;
    }
    return "";
}

bool parse_bool_arg(const std::string& value) {
    std::string v = value;
    for (char& c : v) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    return v == "1" || v == "true" || v == "on" || v == "yes";
}

void apply_config_from_form(const std::string& body) {
    auto maybe_set_string = [&](const char* key, char* target, size_t targetSize) {
        std::string value = get_form_value(body, key);
        if (!value.empty() || body.find(std::string(key) + "=") != std::string::npos) {
            safe_copy(target, targetSize, value);
        }
    };

    std::string val = get_form_value(body, "wifiReconnectMs");
    if (!val.empty()) gConfig.wifiReconnectIntervalMs = static_cast<uint32_t>(std::strtoul(val.c_str(), nullptr, 10));

    maybe_set_string("wifiSsid", gConfig.wifiSsid, sizeof(gConfig.wifiSsid));
    maybe_set_string("wifiPassword", gConfig.wifiPassword, sizeof(gConfig.wifiPassword));
    maybe_set_string("hostName", gConfig.hostName, sizeof(gConfig.hostName));

    val = get_form_value(body, "powerOnByDefault");
    if (!val.empty()) gConfig.powerOnByDefault = parse_bool_arg(val) ? 1 : 0;

    val = get_form_value(body, "uartControl");
    if (!val.empty()) gConfig.uartControl = parse_bool_arg(val) ? 1 : 0;

    val = get_form_value(body, "uartTimeout");
    if (!val.empty()) gConfig.uartTimeout = static_cast<uint32_t>(std::strtoul(val.c_str(), nullptr, 10));

    val = get_form_value(body, "mqttEnabled");
    if (!val.empty()) gConfig.mqttEnabled = parse_bool_arg(val) ? 1 : 0;

    val = get_form_value(body, "mqttPort");
    if (!val.empty()) gConfig.mqttPort = static_cast<uint16_t>(std::strtoul(val.c_str(), nullptr, 10));

    val = get_form_value(body, "mqttHaDiscovery");
    if (!val.empty()) gConfig.mqttHaDiscovery = parse_bool_arg(val) ? 1 : 0;

    maybe_set_string("mqttHost", gConfig.mqttHost, sizeof(gConfig.mqttHost));
    maybe_set_string("mqttUser", gConfig.mqttUser, sizeof(gConfig.mqttUser));
    maybe_set_string("mqttPassword", gConfig.mqttPassword, sizeof(gConfig.mqttPassword));
    maybe_set_string("mqttBaseTopic", gConfig.mqttBaseTopic, sizeof(gConfig.mqttBaseTopic));

    sanitize_config(gConfig);
    save_config_nvs();
}

esp_err_t send_json(httpd_req_t* req, const std::string& text) {
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, text.c_str(), HTTPD_RESP_USE_STRLEN);
}

}  // namespace powerrelay
