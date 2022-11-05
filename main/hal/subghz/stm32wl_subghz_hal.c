#include "stm32wl_subghz_hal.h"
#include "stm32wlxx_ll_utils.h"
#include <stm32wlxx_ll_pwr.h>

// Some weird loop cycles definition from ST
#define SUBGHZ_DEFAULT_LOOP_TIME   ((SystemCoreClock*28U)>>19U)
#define SUBGHZ_RFBUSY_LOOP_TIME    ((SystemCoreClock*24U)>>20U)
#define SUBGHZ_NSS_LOOP_TIME       ((SystemCoreClock*24U)>>16U)
#define SUBGHZ_DEFAULT_TIMEOUT     100U // 100ms
#define SUBGHZ_DUMMY_DATA          0xFFU

static bool subghz_spi_send_byte(uint8_t data)
{
    volatile uint32_t count = SUBGHZ_DEFAULT_TIMEOUT * SUBGHZ_DEFAULT_LOOP_TIME;

    /* Wait until TXE flag is set */
    do {
        if (count == 0U) { return false; }
        count--;
    } while (READ_BIT(SUBGHZSPI->SR, SPI_SR_TXE) != (SPI_SR_TXE));

    /* Transmit Data*/
#if defined (__GNUC__)
    volatile uint8_t *spidr = ((volatile uint8_t *) &SUBGHZSPI->DR);
    *spidr = data;
#else
    *((volatile uint8_t *)&SUBGHZSPI->DR) = data;
#endif /* __GNUC__ */

    /* Handle Rx transmission from SUBGHZSPI peripheral to Radio ****************/
    /* Initialize Timeout */
    count = SUBGHZ_DEFAULT_TIMEOUT * SUBGHZ_DEFAULT_LOOP_TIME;

    /* Wait until RXNE flag is set */
    do {
        if (count == 0U) { return false; }
        count--;
    } while (READ_BIT(SUBGHZSPI->SR, SPI_SR_RXNE) != (SPI_SR_RXNE));

    /* Flush Rx data */
    READ_REG(SUBGHZSPI->DR);
    return true;
}

static bool subghz_spi_recv_byte(uint8_t *out)
{
    if (out == NULL) return false;
    volatile uint32_t count = SUBGHZ_DEFAULT_TIMEOUT * SUBGHZ_DEFAULT_LOOP_TIME;

    /* Wait until TXE flag is set */
    do {
        if (count == 0U) { return false; }
        count--;
    } while (READ_BIT(SUBGHZSPI->SR, SPI_SR_TXE) != (SPI_SR_TXE));

    /* Transmit Data*/
#if defined (__GNUC__)
    volatile uint8_t *spidr = ((volatile uint8_t *) &SUBGHZSPI->DR);
    *spidr = SUBGHZ_DUMMY_DATA;
#else
    *((volatile uint8_t *)&SUBGHZSPI->DR) = SUBGHZ_DUMMY_DATA;
#endif /* __GNUC__ */

    /* Handle Rx transmission from SUBGHZSPI peripheral to Radio ****************/
    /* Initialize Timeout */
    count = SUBGHZ_DEFAULT_TIMEOUT * SUBGHZ_DEFAULT_LOOP_TIME;

    /* Wait until RXNE flag is set */
    do {
        if (count == 0U) { return false; }
        count--;
    } while (READ_BIT(SUBGHZSPI->SR, SPI_SR_RXNE) != (SPI_SR_RXNE));

    /* Retrieve pData */
    *out = (uint8_t) (READ_REG(SUBGHZSPI->DR));
    return true;
}

static bool subghz_wait_on_busy()
{
    volatile uint32_t count = SUBGHZ_DEFAULT_TIMEOUT * SUBGHZ_RFBUSY_LOOP_TIME;
    volatile uint32_t mask = LL_PWR_IsActiveFlag_RFBUSYMS();

    while ((LL_PWR_IsActiveFlag_RFBUSYS() & mask) == 1) {
        mask = LL_PWR_IsActiveFlag_RFBUSYMS();
        if (count == 0) return false;
        count -= 1;
    }

    return true;
}

sx126x_hal_status_t sx126x_hal_write(const void *context, const uint8_t *command, uint16_t command_length, const uint8_t *data, uint16_t data_length)
{
    (void)context;
    volatile uint32_t primask = __get_PRIMASK();
    __disable_irq();

    // Send commands
    // NSS = 0
    LL_PWR_SelectSUBGHZSPI_NSS();
    for (uint16_t idx = 0; idx < command_length; idx += 1) {
        if (!subghz_spi_send_byte(command[idx])) {
            LL_PWR_UnselectSUBGHZSPI_NSS();
            if (!primask) {
                __enable_irq();
            }
            return SX126X_HAL_STATUS_ERROR;
        }
    }

    // Wait if busy
    if (command[0] != RADIO_SET_SLEEP) {
        if (!subghz_wait_on_busy()) {
            LL_PWR_UnselectSUBGHZSPI_NSS();
            if (!primask) {
                __enable_irq();
            }

            return SX126X_HAL_STATUS_ERROR;
        }
    }

    // Send data, if exists
    if (data != NULL && data_length > 0) {
        for (uint16_t idx = 0; idx < data_length; idx += 1) {
            if (!subghz_spi_send_byte(data[idx])) {
                LL_PWR_UnselectSUBGHZSPI_NSS();
                if (!primask) {
                    __enable_irq();
                }

                return SX126X_HAL_STATUS_ERROR;
            }
        }
    }

    // NSS = 1
    LL_PWR_UnselectSUBGHZSPI_NSS();

    // Wait if busy
    if (command[0] != RADIO_SET_SLEEP) {
        if (!subghz_wait_on_busy()) {
            if (!primask) {
                __enable_irq();
            }

            return SX126X_HAL_STATUS_ERROR;
        }
    }

    if (!primask) {
        __enable_irq();
    }

    return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_read(const void *context, const uint8_t *command, uint16_t command_length, uint8_t *data, uint16_t data_length)
{
    (void)context;
    volatile uint32_t primask = __get_PRIMASK();
    __disable_irq();

    // Send commands
    // NSS = 0
    LL_PWR_SelectSUBGHZSPI_NSS();
    for (uint16_t cnt = 0; cnt < command_length; cnt += 1) {
        if (!subghz_spi_send_byte(command[cnt])) {
            LL_PWR_UnselectSUBGHZSPI_NSS();
            if (!primask) {
                __enable_irq();
            }

            return SX126X_HAL_STATUS_ERROR;
        }
    }

    // Wait if busy
    if (command[0] != RADIO_SET_SLEEP) {
        if (!subghz_wait_on_busy()) {
            LL_PWR_UnselectSUBGHZSPI_NSS();
            if (!primask) {
                __enable_irq();
            }
            return SX126X_HAL_STATUS_ERROR;
        }
    }

    // Receive data
    if (data != NULL && data_length > 0) {
        // NSS = 0
        LL_PWR_SelectSUBGHZSPI_NSS();
        for (uint16_t idx = 0; idx < data_length; idx += 1) {
            if (!subghz_spi_recv_byte(data)) {
                LL_PWR_UnselectSUBGHZSPI_NSS();
                if (!primask) {
                    __enable_irq();
                }

                return SX126X_HAL_STATUS_ERROR;
            }

            data += 1;
        }
    }


    // NSS = 1
    LL_PWR_UnselectSUBGHZSPI_NSS();

    if (!primask) {
        __enable_irq();
    }

    return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_reset(const void *context)
{
    (void)context;

    // According to STM32WL ref manual, RCC->RFRST at boot is 1, which means the nRESET for SX126x is asserted by default
    if (!LL_RCC_RF_IsEnabledReset()) {
        LL_RCC_RF_EnableReset();
        LL_mDelay(1);
    }

    LL_RCC_RF_DisableReset();
    while (LL_RCC_IsRFUnderReset()) {}

    return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_wakeup(const void *context)
{
    (void)context;

    // See SX1262 datasheet, Chapter 9.3 "Sleep Mode" - assert NSS to wake up
    LL_PWR_SelectSUBGHZSPI_NSS();

    volatile uint32_t count = SUBGHZ_NSS_LOOP_TIME;
    do {
        count -= 1;
    } while (count != 0);


    LL_PWR_UnselectSUBGHZSPI_NSS();
    return SX126X_HAL_STATUS_OK;
}