#include <stdio.h>
#include <stm32wlxx_ll_usart.h>
#include <stm32wlxx_ll_gpio.h>
#include <stm32wlxx_ll_bus.h>
#include <stm32wlxx_ll_rcc.h>
#include <printf.h>
#include "log.h"

void logger_init()
{
#ifndef DISABLE_LOG
    LL_USART_InitTypeDef usart_config = {0};
    LL_GPIO_InitTypeDef gpio_config = {0};

    LL_RCC_SetUSARTClockSource(LL_RCC_USART2_CLKSOURCE_HSI);
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);

    if (!LL_AHB2_GRP1_IsEnabledClock(LL_AHB2_GRP1_PERIPH_GPIOB)) {
        LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
    }

    if (!LL_AHB2_GRP1_IsEnabledClockSleep(LL_AHB2_GRP1_PERIPH_GPIOB)) {
        LL_AHB2_GRP1_EnableClockSleep(LL_AHB2_GRP1_PERIPH_GPIOB);
    }

    gpio_config.Pin = LL_GPIO_PIN_7 | LL_GPIO_PIN_6; // PB7 = Rx, PB6 = Tx
    gpio_config.Mode = LL_GPIO_MODE_ALTERNATE;
    gpio_config.Speed = LL_GPIO_SPEED_FREQ_LOW;
    gpio_config.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    gpio_config.Pull = LL_GPIO_PULL_NO;
    gpio_config.Alternate = LL_GPIO_AF_7;
    LL_GPIO_Init(GPIOB, &gpio_config);

    usart_config.PrescalerValue = LL_USART_PRESCALER_DIV1;
    usart_config.BaudRate = 921600;
    usart_config.DataWidth = LL_USART_DATAWIDTH_8B;
    usart_config.StopBits = LL_USART_STOPBITS_2;
    usart_config.Parity = LL_USART_PARITY_NONE;
    usart_config.TransferDirection = LL_USART_DIRECTION_TX;
    usart_config.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    usart_config.OverSampling = LL_USART_OVERSAMPLING_16;
    LL_USART_Init(USART1, &usart_config);
    LL_USART_SetTXFIFOThreshold(USART1, LL_USART_FIFOTHRESHOLD_1_8);
    LL_USART_SetRXFIFOThreshold(USART1, LL_USART_FIFOTHRESHOLD_1_8);
    LL_USART_DisableFIFO(USART1);
    LL_USART_ConfigAsyncMode(USART1);

    LL_USART_Enable(USART1);

    while(!(LL_USART_IsActiveFlag_TEACK(USART1)))
    {
    }
#endif
}

void putchar_(char c)
{
#ifndef DISABLE_LOG
    while(!LL_USART_IsActiveFlag_TXE_TXFNF(USART1)) {}
    LL_USART_TransmitData8(USART1, c);
#endif
}
