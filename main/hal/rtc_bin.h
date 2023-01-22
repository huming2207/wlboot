#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stm32wlxx_ll_rtc.h>

void stm32wl_rtc_bin_init(uint8_t prescaler);
uint32_t stm32wl_rtc_bin_get_ctr();
void stm32wl_rtc_bin_set_alarm_a(uint32_t tick);
void stm32wl_rtc_bin_set_alarm_b(uint32_t tick);
inline void stm32wl_rtc_bin_enable_alarm_a();
inline void stm32wl_rtc_bin_enable_alarm_b();
inline void stm32wl_rtc_bin_disable_alarm_a();
inline void stm32wl_rtc_bin_disable_alarm_b();

#ifdef __cplusplus
}
#endif