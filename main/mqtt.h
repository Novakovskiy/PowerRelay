#pragma once

namespace powerrelay {

void mqtt_disconnect();
void mqtt_publish_state(bool force);
void mqtt_ensure_connected();

}  // namespace powerrelay

