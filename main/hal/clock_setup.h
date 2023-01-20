#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void setup_high_speed_clk();
void setup_low_speed_clk();
void setup_flash_latency();
void setup_system_clk();

void clock_init();

#ifdef __cplusplus
}
#endif