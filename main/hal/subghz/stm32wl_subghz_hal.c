#include "stm32wl_subghz_hal.h"
#include "stm32wlxx_ll_utils.h"
#include <stm32wlxx_hal_subghz.h>
#include <stm32wlxx_ll_pwr.h>

// Some weird loop cycles definition from ST
#define SUBGHZ_DEFAULT_LOOP_TIME   ((SystemCoreClock*28U)>>19U)
#define SUBGHZ_RFBUSY_LOOP_TIME    ((SystemCoreClock*24U)>>20U)
#define SUBGHZ_NSS_LOOP_TIME       ((SystemCoreClock*24U)>>16U)


sx126x_hal_status_t sx126x_hal_write(const void* context, const uint8_t* command, uint16_t command_length, const uint8_t* data, uint16_t data_length)
{

}

sx126x_hal_status_t sx126x_hal_read(const void* context, const uint8_t* command, uint16_t command_length, uint8_t* data, uint16_t data_length)
{

}

sx126x_hal_status_t sx126x_hal_reset(const void* context)
{
    // According to STM32WL ref manual, RCC->RFRST at boot is 1, which means the nRESET for SX126x is asserted by default
    if (!LL_RCC_RF_IsEnabledReset()) {
        LL_RCC_RF_EnableReset();
        LL_mDelay(1);
    }

    LL_RCC_RF_DisableReset();
    while (LL_RCC_IsRFUnderReset()) {}

    return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_wakeup(const void* context)
{
    // See SX1262 datasheet, Chapter 9.3 "Sleep Mode" - assert NSS to wake up
    LL_PWR_SelectSUBGHZSPI_NSS();

    volatile uint32_t count  = SUBGHZ_NSS_LOOP_TIME;
    do {
        count -= 1;
    } while (count != 0);


    LL_PWR_UnselectSUBGHZSPI_NSS();
    return SX126X_HAL_STATUS_OK;
}