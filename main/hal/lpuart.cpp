#include <cstring>
#include <stm32wlxx_ll_lpuart.h>
#include <stm32wlxx_ll_gpio.h>
#include <stm32wlxx_ll_rcc.h>
#include <stm32wlxx_ll_bus.h>

#include "lpuart.hpp"


bool lpuart::init()
{
    LL_LPUART_InitTypeDef lpuart_cfg = {0};
    LL_GPIO_InitTypeDef gpio_cfg = {0};

    LL_RCC_SetLPUARTClockSource(LL_RCC_LPUART1_CLKSOURCE_LSE);

    /* Peripheral clock enable */
    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_LPUART1);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);

    /**
     * LPUART1 GPIO Configuration
        PA2   ------> LPUART1_TX
        PA3   ------> LPUART1_RX
    */
    gpio_cfg.Pin = LL_GPIO_PIN_2 | LL_GPIO_PIN_3;
    gpio_cfg.Mode = LL_GPIO_MODE_ALTERNATE;
    gpio_cfg.Speed = LL_GPIO_SPEED_FREQ_LOW;
    gpio_cfg.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    gpio_cfg.Pull = LL_GPIO_PULL_NO;
    gpio_cfg.Alternate = LL_GPIO_AF_8;
    LL_GPIO_Init(GPIOA, &gpio_cfg);

    NVIC_SetPriority(LPUART1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
    NVIC_EnableIRQ(LPUART1_IRQn);

    lpuart_cfg.PrescalerValue = LL_LPUART_PRESCALER_DIV1;
    lpuart_cfg.BaudRate = 9600;
    lpuart_cfg.DataWidth = LL_LPUART_DATAWIDTH_8B;
    lpuart_cfg.StopBits = LL_LPUART_STOPBITS_1;
    lpuart_cfg.Parity = LL_LPUART_PARITY_NONE;
    lpuart_cfg.TransferDirection = LL_LPUART_DIRECTION_TX_RX;
    lpuart_cfg.HardwareFlowControl = LL_LPUART_HWCONTROL_NONE;
    LL_LPUART_Init(LPUART1, &lpuart_cfg);
    LL_LPUART_SetTXFIFOThreshold(LPUART1, LL_LPUART_FIFOTHRESHOLD_1_8);
    LL_LPUART_SetRXFIFOThreshold(LPUART1, LL_LPUART_FIFOTHRESHOLD_1_8);
    LL_LPUART_EnableIT_TXE(LPUART1);
    LL_LPUART_EnableIT_RXNE_RXFNE(LPUART1);
    LL_LPUART_EnableInStopMode(LPUART1);

    LL_LPUART_Enable(LPUART1);
    while((!(LL_LPUART_IsActiveFlag_TEACK(LPUART1))) || (!(LL_LPUART_IsActiveFlag_REACK(LPUART1))))
    {
    }

    LFBB_Init(&tx_fifo, tx_buf, sizeof(tx_buf));
    LFBB_Init(&rx_fifo, rx_buf, sizeof(rx_buf));

    return true;
}

bool lpuart::transmit(uint8_t *buf, size_t len)
{
    if (len > sizeof(tx_buf)) {
        return false; // Too big for Tx FIFO
    }

    auto *acq_buf= LFBB_WriteAcquire(&tx_fifo, len);
    if (acq_buf == nullptr) {
        return false;
    }

    memcpy(acq_buf, buf, len);
    LFBB_WriteRelease(&tx_fifo, len);
    return true;
}

void lpuart::on_pkt_recv(uint8_t *buf, size_t len)
{

}

void lpuart::handle_task()
{
    if (rx_avail) {
        auto *acq_buf = LFBB_WriteAcquire(&rx_fifo, 1);
        *acq_buf = LL_LPUART_ReceiveData8(LPUART1);
        LFBB_WriteRelease(&rx_fifo, 1);
    }

    if (tx_avail) {
        size_t _avail_len = 0;
        auto *data = LFBB_ReadAcquire(&tx_fifo, &_avail_len);
        LL_LPUART_TransmitData8(LPUART1, *data);
        LFBB_ReadRelease(&tx_fifo, 1);
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

void LPUART1_IRQHandler()
{
    lpuart::instance()->on_intr();
}
