#pragma once

#include <string>

#include "esp_err.h"
#include "esp_http_server.h"

namespace powerrelay {

std::string json_from_config();
void apply_config_from_form(const std::string& body);
esp_err_t send_json(httpd_req_t* req, const std::string& text);

}  // namespace powerrelay
