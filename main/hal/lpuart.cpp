#include <cstring>
#include <stm32wlxx_ll_lpuart.h>
#include <stm32wlxx_ll_gpio.h>
#include <stm32wlxx_ll_rcc.h>
#include <stm32wlxx_ll_bus.h>

#include "lpuart.hpp"
#include "log.h"

LfBb<uint8_t, 1024> lpuart::rx_buf = {};

volatile int32_t lpuart::recv_byte = -1;
volatile bool lpuart::framing_error = false;
volatile bool lpuart::noise_error = false;
volatile bool lpuart::overrun_error = false;
volatile bool lpuart::parity_error = false;

bool lpuart::init()
{
    LL_LPUART_InitTypeDef LPUART_InitStruct = {0};

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    LL_RCC_SetLPUARTClockSource(LL_RCC_LPUART1_CLKSOURCE_LSE);

    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_LPUART1);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_2|LL_GPIO_PIN_3;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_8;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* LPUART1 interrupt Init */
    NVIC_SetPriority(LPUART1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
    NVIC_EnableIRQ(LPUART1_IRQn);

    /* USER CODE BEGIN LPUART1_Init 1 */

    /* USER CODE END LPUART1_Init 1 */
    LPUART_InitStruct.PrescalerValue = LL_LPUART_PRESCALER_DIV1;
    LPUART_InitStruct.BaudRate = 9600;
    LPUART_InitStruct.DataWidth = LL_LPUART_DATAWIDTH_8B;
    LPUART_InitStruct.StopBits = LL_LPUART_STOPBITS_1;
    LPUART_InitStruct.Parity = LL_LPUART_PARITY_NONE;
    LPUART_InitStruct.TransferDirection = LL_LPUART_DIRECTION_TX_RX;
    LPUART_InitStruct.HardwareFlowControl = LL_LPUART_HWCONTROL_NONE;
    LL_LPUART_Init(LPUART1, &LPUART_InitStruct);
    LL_LPUART_SetTXFIFOThreshold(LPUART1, LL_LPUART_FIFOTHRESHOLD_1_8);
    LL_LPUART_SetRXFIFOThreshold(LPUART1, LL_LPUART_FIFOTHRESHOLD_1_8);
    LL_LPUART_DisableFIFO(LPUART1);

    LL_LPUART_Enable(LPUART1);

    /* Polling LPUART1 initialisation */
    while((!(LL_LPUART_IsActiveFlag_TEACK(LPUART1))) || (!(LL_LPUART_IsActiveFlag_REACK(LPUART1))))
    {
    }

    LL_LPUART_ClearFlag_ORE(LPUART1);
    LL_LPUART_EnableIT_RXNE_RXFNE(LPUART1);
    LL_LPUART_EnableIT_ERROR(LPUART1);
    return true;
}

bool lpuart::transmit(uint8_t *buf, size_t len)
{
    if (buf == nullptr) return false;

    for (size_t idx = 0; idx < len; idx += 1) {
        while (!LL_LPUART_IsActiveFlag_TXE_TXFNF(LPUART1)) {}

        if (idx == len - 1) {
            LL_LPUART_ClearFlag_TC(LPUART1);
        }

        LL_LPUART_TransmitData8(LPUART1, buf[idx]);
    }

    while (!LL_LPUART_IsActiveFlag_TC(LPUART1)) {}
    return true;
}

uint8_t *lpuart::begin_read_rx_buf(size_t buf_len, size_t *avail_len)
{
    auto ret = rx_buf.ReadAcquire();
    if (avail_len != nullptr) {
        *avail_len = ret.second;
    }

    return ret.first;
}

void lpuart::handle_task()
{
    if (recv_byte != -1) {
        volatile uint8_t byte_recved = recv_byte & 0xff;
        recv_byte = -1;
        auto *acq_buf = rx_buf.WriteAcquire(1);
        *acq_buf = byte_recved;
        rx_buf.WriteRelease(1);
    }
}

void lpuart::set_tx(bool enable)
{
    if (enable) {
        LL_LPUART_EnableDirectionTx(LPUART1);
    } else {
        LL_LPUART_DisableDirectionTx(LPUART1);
    }
}

void lpuart::set_rx(bool enable)
{
    if (enable) {
        LL_LPUART_EnableDirectionRx(LPUART1);
    } else {
        LL_LPUART_DisableDirectionRx(LPUART1);
    }
}

void lpuart::done_read_rx_buf(size_t len)
{
    rx_buf.ReadRelease(len);
}

size_t lpuart::get_rx_buf_len()
{
    auto ret = rx_buf.ReadAcquire();
    rx_buf.ReadRelease(0);

    return ret.second;
}

extern "C" void LPUART1_IRQHandler()
{
    if (LL_LPUART_IsActiveFlag_RXNE_RXFNE(LPUART1)) {
        lpuart::recv_byte = LL_LPUART_ReceiveData8(LPUART1);
    }

    if (LL_LPUART_IsActiveFlag_FE(LPUART1)) {
        LL_LPUART_ClearFlag_FE(LPUART1);
        lpuart::framing_error = true;
    }

    if (LL_LPUART_IsActiveFlag_NE(LPUART1)) {
        LL_LPUART_ClearFlag_NE(LPUART1);
        lpuart::noise_error = true;
    }

    if (LL_LPUART_IsActiveFlag_ORE(LPUART1)) {
        LL_LPUART_ClearFlag_ORE(LPUART1);
        lpuart::overrun_error = true;
    }

    if (LL_LPUART_IsActiveFlag_PE(LPUART1)) {
        LL_LPUART_ClearFlag_PE(LPUART1);
        lpuart::parity_error = true;
    }
}
