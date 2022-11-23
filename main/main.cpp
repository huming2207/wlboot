#include <cstdio>
#include "stm32wlxx_ll_utils.h"
#include "subghz.hpp"
#include "log.h"
#include "stm32wlxx_ll_pwr.h"
#include "stm32wlxx_ll_system.h"
#include "stm32wlxx_ll_rcc.h"
#include "stm32wlxx_ll_bus.h"

#include <lpuart.hpp>

#ifndef DISABLE_LOG
extern "C" void initialise_monitor_handles(void);
#endif

static void SystemClock_Config()
{
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);
    while(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_2)
    {
    }

    LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
    LL_RCC_MSI_Enable();

    /* Wait till MSI is ready */
    while(LL_RCC_MSI_IsReady() != 1)
    {
    }

    LL_RCC_MSI_EnableRangeSelection();
    LL_RCC_MSI_SetRange(LL_RCC_MSIRANGE_11);
    LL_RCC_MSI_SetCalibTrimming(0);
    LL_PWR_EnableBkUpAccess();
    LL_RCC_LSE_SetDriveCapability(LL_RCC_LSEDRIVE_HIGH);
    LL_RCC_LSE_Enable();

    /* Wait till LSE is ready */
    while(LL_RCC_LSE_IsReady() != 1)
    {
    }

    LL_RCC_LSE_EnablePropagation(); // Enable LSE output to peripherals, like LPUART etc.
    LL_RCC_MSI_EnablePLLMode();
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_MSI);

    /* Wait till System clock is ready */
    while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_MSI)
    {
    }

    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAHB3Prescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);

    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_CRC);

    /* Update CMSIS variable (which can be updated also through SystemCoreClockUpdate function) */
    LL_SetSystemCoreClock(48000000);
    LL_Init1msTick(48000000);
}

int main()
{
#ifndef DISABLE_LOG
    initialise_monitor_handles();
#endif

    SystemClock_Config();
    auto *uart = lpuart::instance();
    auto *lora= subghz::instance();

    uart->init();
    lora->init();

    if (!lora->setup_lora(918000000, SX126X_LORA_SF7, SX126X_LORA_BW_125, false)) {
        WLB_LOG("LoRa mode init failed");
    }

    uint8_t buf[8] = { 0xca, 0xfe, 0xbe, 0xef, 0x5a, 0xa5, 0xaa, 0x55 };
    while (true) {
        uart->handle_task();
        lora->handle_task();
        lora->set_lora_tx(buf, sizeof(buf), 20, 1000, 20, true);
        LL_mDelay(300);
    }

    uint8_t rx_buf[8] = {};
    while (true) {
        uart->handle_task();
        lora->handle_task();
        lora->set_lora_rx(sizeof(rx_buf), 1000, 20, true, true);
        LL_mDelay(1000);
    }
}

