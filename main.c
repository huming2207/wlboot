#include <stdio.h>
#include <stdint.h>
#include <stm32wle5xx.h>
#include <stm32wlxx_ll_rcc.h>
#include <stm32wlxx_ll_pwr.h>
#include <stm32wlxx_ll_system.h>
#include <stm32wlxx_ll_utils.h>

static void setup_clock()
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
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_MSI);

    /* Wait till System clock is ready */
    while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_MSI)
    {
    }

    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAHB3Prescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);

    LL_Init1msTick(48000000);

    /* Update CMSIS variable (which can be updated also through SystemCoreClockUpdate function) */
    LL_SetSystemCoreClock(48000000);
}

int main()
{
    setup_clock();
    printf("Hello, World!\n");
    return 0;
}
