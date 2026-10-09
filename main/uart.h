#pragma once

namespace powerrelay {

void uart_control_init();
void uart_control_poll();
void uart_control_send_power_state(bool on);

}  // namespace powerrelay
