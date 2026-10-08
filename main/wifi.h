#pragma once

#include <cstdint>

#include "esp_event_base.h"

namespace powerrelay {

void apply_wifi_mode();
void wifi_event_handler(void*, esp_event_base_t event_base, int32_t event_id, void*);
void maybe_retry_wifi();

}  // namespace powerrelay


