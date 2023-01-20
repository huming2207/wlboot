#include <stm32wlxx_ll_rcc.h>
#include <stm32wlxx_ll_pwr.h>
#include <stm32wlxx_ll_bus.h>
#include <stm32wlxx_ll_system.h>

#include "rtc_bin.h"

void stm32wl_rtc_bin_init(uint8_t prescaler)
{
    LL_RCC_EnableRTC();
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_RTCAPB);

    LL_RTC_InitTypeDef rtc_config = {};
    rtc_config.AsynchPrescaler = (prescaler & 0x7f);
    LL_RTC_Init(RTC, &rtc_config);
    LL_RTC_SetBinaryMode(RTC, LL_RTC_BINARY_ONLY);
}

uint32_t stm32wl_rtc_bin_get_ctr()
{
    return LL_RTC_TS_GetSubSecond(RTC);
}

void stm32wl_rtc_bin_set_alarm_a(uint32_t tick)
{
    LL_RTC_ALMA_Disable(RTC);
    LL_RTC_ALMA_SetBinAutoClr(RTC, tick);
    LL_RTC_ALMA_Enable(RTC);
}

void stm32wl_rtc_bin_set_alarm_b(uint32_t tick)
{
    LL_RTC_ALMB_Disable(RTC);
    LL_RTC_ALMB_SetBinAutoClr(RTC, tick);
    LL_RTC_ALMB_Enable(RTC);
}

void stm32wl_rtc_bin_enable_alarm_a()
{
    LL_RTC_ALMA_Enable(RTC);
}

void stm32wl_rtc_bin_enable_alarm_b()
{
    LL_RTC_ALMB_Enable(RTC);
}

void stm32wl_rtc_bin_disable_alarm_a()
{
    LL_RTC_ALMA_Disable(RTC);
}

void stm32wl_rtc_bin_disable_alarm_b()
{
    LL_RTC_ALMB_Disable(RTC);
}
