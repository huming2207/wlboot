#include <stm32wlxx_ll_utils.h>
#include <stm32wlxx_ll_pwr.h>
#include <stddef.h>

#include "sx126x.h"
#include "stm32wl_subghz_hal.h"
#include "stm32wlxx_ll_exti.h"
#include "stm32wlxx_ll_bus.h"
#include "stm32wlxx_ll_rcc.h"

// Some weird loop cycles definition from ST
#define SUBGHZ_DEFAULT_LOOP_TIME   ((SystemCoreClock*28U)>>19U)
#define SUBGHZ_RFBUSY_LOOP_TIME    ((SystemCoreClock*24U)>>20U)
#define SUBGHZ_NSS_LOOP_TIME       ((SystemCoreClock*24U)>>16U)
#define SUBGHZ_DEFAULT_TIMEOUT     100U // 100ms
#define SUBGHZ_DUMMY_DATA          0xFFU

#define SUBGHZSPI_BAUDRATEPRESCALER_2       (0x00000000U)
#define SUBGHZSPI_BAUDRATEPRESCALER_4       (SPI_CR1_BR_0)
#define SUBGHZSPI_BAUDRATEPRESCALER_8       (SPI_CR1_BR_1)
#define SUBGHZSPI_BAUDRATEPRESCALER_16      (SPI_CR1_BR_1 | SPI_CR1_BR_0)
#define SUBGHZSPI_BAUDRATEPRESCALER_32      (SPI_CR1_BR_2)
#define SUBGHZSPI_BAUDRATEPRESCALER_64      (SPI_CR1_BR_2 | SPI_CR1_BR_0)
#define SUBGHZSPI_BAUDRATEPRESCALER_128     (SPI_CR1_BR_2 | SPI_CR1_BR_1)
#define SUBGHZSPI_BAUDRATEPRESCALER_256     (SPI_CR1_BR_2 | SPI_CR1_BR_1 | SPI_CR1_BR_0)

#define SX126X_CMD_SET_SLEEP 0x84

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

sx126x_hal_status_t sx126x_hal_write(const uint8_t *command, uint16_t command_length, const uint8_t *data, uint16_t data_length)
{
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
    if (command[0] != SX126X_CMD_SET_SLEEP) {
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
    if (command[0] != SX126X_CMD_SET_SLEEP) {
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

sx126x_hal_status_t sx126x_hal_read(const uint8_t *command, uint16_t command_length, uint8_t *data, uint16_t data_length)
{
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
    if (command[0] != SX126X_CMD_SET_SLEEP) {
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

bool stm32wl_subghz_init()
{
    LL_APB3_GRP1_EnableClock(LL_APB3_GRP1_PERIPH_SUBGHZSPI);
    if (!LL_RCC_HSE_IsReady()) {
#ifdef STM32WL_HAS_TCXO
        LL_RCC_HSE_EnableTcxo();
#endif
        LL_RCC_HSE_Enable();
        while (LL_RCC_HSE_IsReady() == 0) {}
    }

    NVIC_SetPriority(SUBGHZ_Radio_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
    NVIC_EnableIRQ(SUBGHZ_Radio_IRQn);

    sx126x_hal_reset(NULL);

    /* Asserts the reset signal of the Radio peripheral */
    LL_PWR_UnselectSUBGHZSPI_NSS();

    /* Enable EXTI 44 : Radio IRQ ITs for CPU1 */
    LL_EXTI_EnableIT_32_63(LL_EXTI_LINE_44);

    /* Enable wakeup signal of the Radio peripheral */
    LL_PWR_SetRadioBusyTrigger(LL_PWR_RADIO_BUSY_TRIGGER_WU_IT);

    /* Clear Pending Flag */
    LL_PWR_ClearFlag_RFBUSY();

    // Initialise SPI
    /* Disable SUBGHZSPI Peripheral */
    CLEAR_BIT(SUBGHZSPI->CR1, SPI_CR1_SPE);

    /*----------------------- SPI CR1 Configuration ----------------------------*
     *             SPI Mode: Master                                             *
     *   Communication Mode: 2 lines (Full-Duplex)                              *
     *       Clock polarity: Low                                                *
     *                phase: 1st Edge                                           *
     *       NSS management: Internal (Done with External bit inside PWR        *
     *  Communication speed: BaudratePrescaler                             *
     *            First bit: MSB                                                *
     *      CRC calculation: Disable                                            *
     *--------------------------------------------------------------------------*/
    WRITE_REG(SUBGHZSPI->CR1, (SPI_CR1_MSTR | SPI_CR1_SSI | SUBGHZSPI_BAUDRATEPRESCALER_2 | SPI_CR1_SSM));

    /*----------------------- SPI CR2 Configuration ----------------------------*
     *            Data Size: 8bits                                              *
     *              TI Mode: Disable                                            *
     *            NSS Pulse: Disable                                            *
     *    Rx FIFO Threshold: 8bits                                              *
     *--------------------------------------------------------------------------*/
    WRITE_REG(SUBGHZSPI->CR2, (SPI_CR2_FRXTH |  SPI_CR2_DS_0 | SPI_CR2_DS_1 | SPI_CR2_DS_2));

    /* Enable SUBGHZSPI Peripheral */
    SET_BIT(SUBGHZSPI->CR1, SPI_CR1_SPE);
    return true;
}

bool stm32wl_subghz_deinit()
{
    /* Disable SUBGHZSPI Peripheral */
    CLEAR_BIT(SUBGHZSPI->CR1, SPI_CR1_SPE);

    LL_APB3_GRP1_DisableClock(LL_APB3_GRP1_PERIPH_SUBGHZSPI);
    NVIC_DisableIRQ(SUBGHZ_Radio_IRQn);

    /* Disable EXTI 44 : Radio IRQ ITs for CPU1 */
    LL_EXTI_DisableIT_32_63(LL_EXTI_LINE_44);

    /* Disable wakeup signal of the Radio peripheral */
    LL_PWR_SetRadioBusyTrigger(LL_PWR_RADIO_BUSY_TRIGGER_NONE);

    /* Clear Pending Flag */
    LL_PWR_ClearFlag_RFBUSY();

    /* Re-asserts the reset signal of the Radio peripheral */
    LL_RCC_RF_EnableReset();

    return true;
}
