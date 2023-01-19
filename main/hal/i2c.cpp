#include <stm32wlxx_ll_i2c.h>

#include "i2c.hpp"
#include "stm32wlxx_ll_gpio.h"
#include "stm32wlxx_ll_rcc.h"
#include "stm32wlxx_ll_bus.h"

void i2c::enable_10bit_mode(bool enable)
{
    addr_10bit = false;
}

bool i2c::send_poll(uint16_t addr, const uint8_t *buf, uint8_t len)
{
    auto *buf_ptr = (uint8_t *)buf;
    uint32_t addr_bit_mode = addr_10bit ? LL_I2C_ADDRESSING_MODE_10BIT : LL_I2C_ADDRESSING_MODE_7BIT;
    LL_I2C_HandleTransfer(i2c_periph, addr, addr_bit_mode, len, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);

    int32_t timeout_ctr = len * 3; // JMH: need to clean this shit later
    while (timeout_ctr > 0 && !LL_I2C_IsActiveFlag_STOP(i2c_periph)) {
        if (LL_I2C_IsActiveFlag_TXIS(i2c_periph)) {
            LL_I2C_TransmitData8(i2c_periph, (*buf_ptr++));
        } else {
            timeout_ctr -= 1;
        }
    }

    LL_I2C_ClearFlag_STOP(i2c_periph);
    return timeout_ctr > 0;
}

bool i2c::send_recv_poll(uint16_t addr, const uint8_t *buf, uint8_t len, uint8_t *out, size_t max_out_len, size_t *actual_out_len)
{
    if (!send_poll(addr, buf, len)) {
        return false;
    }

    uint32_t addr_bit_mode = addr_10bit ? LL_I2C_ADDRESSING_MODE_10BIT : LL_I2C_ADDRESSING_MODE_7BIT;
    LL_I2C_HandleTransfer(i2c_periph, addr, addr_bit_mode, len, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_READ);

    size_t out_cnt = max_out_len;
    uint8_t *out_ptr = out;

    int32_t timeout_ctr = (int)max_out_len * 3;
    size_t actual_out_ctr = 0;
    while (timeout_ctr > 0 && (!LL_I2C_IsActiveFlag_STOP(i2c_periph) || out_cnt > 0)) {
        if (LL_I2C_IsActiveFlag_RXNE(i2c_periph)) {
            (*out_ptr++) = LL_I2C_ReceiveData8(i2c_periph);
            out_cnt -= 1;
            actual_out_ctr += 1;
        } else {
            timeout_ctr -= 1;
        }
    }

    LL_I2C_ClearFlag_STOP(i2c_periph);
    if (actual_out_len != nullptr) {
        *actual_out_len = actual_out_ctr;
    }

    return timeout_ctr > 0;
}

uint8_t *i2c::begin_read_rx_buf(size_t *avail_len)
{
    auto ret = rx_buf.ReadAcquire();
    if (avail_len != nullptr) {
        *avail_len = ret.second;
    }

    return ret.first;
}

void i2c::done_read_rx_buf(size_t read_len)
{
    rx_buf.ReadRelease(read_len);
}

size_t i2c::get_rx_buf_len()
{
    auto ret = rx_buf.ReadAcquire();
    rx_buf.ReadRelease(0);

    return ret.second;
}

void i2c::handle_task()
{
    // No-op for now?
}

void i2c::recv(uint16_t addr, size_t expected_len)
{
    uint32_t addr_bit_mode = addr_10bit ? LL_I2C_ADDRESSING_MODE_10BIT : LL_I2C_ADDRESSING_MODE_7BIT;
    LL_I2C_EnableIT_ADDR(i2c_periph);
    LL_I2C_EnableIT_ERR(i2c_periph);
    LL_I2C_EnableIT_STOP(i2c_periph);

    LL_I2C_HandleTransfer(i2c_periph, addr, addr_bit_mode, expected_len, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_READ);
}

bool i2c1::init(bool fast_mode, bool enable_pull, i2c_def::timing_preset timing)
{
    i2c_periph = I2C1;
    LL_GPIO_InitTypeDef gpio_config = {};

    LL_RCC_SetI2CClockSource(LL_RCC_I2C1_CLKSOURCE_SYSCLK);
    if (!LL_AHB2_GRP1_IsEnabledClock(LL_AHB2_GRP1_PERIPH_GPIOB)) {
        LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
    }

    gpio_config.Pin = LL_GPIO_PIN_6 | LL_GPIO_PIN_7;
    gpio_config.Mode = LL_GPIO_MODE_ALTERNATE;
    gpio_config.Speed = LL_GPIO_SPEED_FREQ_LOW;
    gpio_config.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
    gpio_config.Pull = enable_pull ? LL_GPIO_PULL_UP : LL_GPIO_PULL_NO;
    gpio_config.Alternate = LL_GPIO_AF_4;
    LL_GPIO_Init(GPIOB, &gpio_config);

    if (!LL_APB1_GRP1_IsEnabledClock(LL_APB1_GRP1_PERIPH_I2C1)) {
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);
    }

    NVIC_SetPriority(I2C1_EV_IRQn, 0);
    NVIC_EnableIRQ(I2C1_EV_IRQn);

    LL_I2C_EnableAutoEndMode(i2c_periph);
    LL_I2C_DisableOwnAddress2(i2c_periph);
    LL_I2C_DisableGeneralCall(i2c_periph);
    LL_I2C_EnableClockStretching(i2c_periph);

    LL_I2C_InitTypeDef i2c_config = {};
    i2c_config.PeripheralMode = LL_I2C_MODE_I2C; // I2C instead of SMBUS
    i2c_config.Timing = timing;
    i2c_config.AnalogFilter = LL_I2C_ANALOGFILTER_ENABLE;
    i2c_config.DigitalFilter = 0x00;
    i2c_config.OwnAddress1 = 0;
    i2c_config.TypeAcknowledge = LL_I2C_ACK;
    i2c_config.OwnAddrSize = LL_I2C_OWNADDRESS1_7BIT;

    LL_I2C_Init(i2c_periph, &i2c_config);
    LL_I2C_SetOwnAddress2(i2c_periph, 0, LL_I2C_OWNADDRESS2_NOMASK);

    return true;
}

bool i2c1::send_poll(uint16_t addr, const uint8_t *buf, uint8_t len)
{
    return i2c::send_poll(addr, buf, len);
}

bool i2c1::send_recv_poll(uint16_t addr, const uint8_t *buf, uint8_t len, uint8_t *out, size_t max_out_len, size_t *actual_out_len)
{
    return i2c::send_recv_poll(addr, buf, len, out, max_out_len, actual_out_len);
}

void i2c1::recv(uint16_t addr, size_t expected_len)
{
    i2c::recv(addr, expected_len);
}

uint8_t *i2c1::begin_read_rx_buf(size_t *avail_len)
{
    return i2c::begin_read_rx_buf(avail_len);
}

void i2c1::done_read_rx_buf(size_t read_len)
{
    i2c::done_read_rx_buf(read_len);
}

size_t i2c1::get_rx_buf_len()
{
    return i2c::get_rx_buf_len();
}

void i2c1::handle_task()
{
    i2c::handle_task();
    if (recv_byte != -1) {
        volatile uint8_t byte_recved = recv_byte & 0xff;
        recv_byte = -1;
        auto *acq_buf = rx_buf.WriteAcquire(1);
        *acq_buf = byte_recved;
        rx_buf.WriteRelease(1);
    }
}


bool i2c2::init(bool fast_mode, bool enable_pull, i2c_def::timing_preset timing)
{
    i2c_periph = I2C2;
    LL_GPIO_InitTypeDef gpio_config = {};

    LL_RCC_SetI2CClockSource(LL_RCC_I2C2_CLKSOURCE_SYSCLK);
    if (!LL_AHB2_GRP1_IsEnabledClock(LL_AHB2_GRP1_PERIPH_GPIOA)) {
        LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
    }

    gpio_config.Pin = LL_GPIO_PIN_12 | LL_GPIO_PIN_11;
    gpio_config.Mode = LL_GPIO_MODE_ALTERNATE;
    gpio_config.Speed = LL_GPIO_SPEED_FREQ_LOW;
    gpio_config.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
    gpio_config.Pull = enable_pull ? LL_GPIO_PULL_UP : LL_GPIO_PULL_NO;
    gpio_config.Alternate = LL_GPIO_AF_4;
    LL_GPIO_Init(GPIOA, &gpio_config);

    if (!LL_APB1_GRP1_IsEnabledClock(LL_APB1_GRP1_PERIPH_I2C2)) {
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C2);
    }

    NVIC_SetPriority(I2C2_EV_IRQn, 0);
    NVIC_EnableIRQ(I2C2_EV_IRQn);

    LL_I2C_EnableAutoEndMode(i2c_periph);
    LL_I2C_DisableOwnAddress2(i2c_periph);
    LL_I2C_DisableGeneralCall(i2c_periph);
    LL_I2C_EnableClockStretching(i2c_periph);

    LL_I2C_InitTypeDef i2c_config = {};
    i2c_config.PeripheralMode = LL_I2C_MODE_I2C; // I2C instead of SMBUS
    i2c_config.Timing = timing;
    i2c_config.AnalogFilter = LL_I2C_ANALOGFILTER_ENABLE;
    i2c_config.DigitalFilter = 0x00;
    i2c_config.OwnAddress1 = 0;
    i2c_config.TypeAcknowledge = LL_I2C_ACK;
    i2c_config.OwnAddrSize = LL_I2C_OWNADDRESS1_7BIT;

    LL_I2C_Init(i2c_periph, &i2c_config);
    LL_I2C_SetOwnAddress2(i2c_periph, 0, LL_I2C_OWNADDRESS2_NOMASK);

    return true;
}

bool i2c2::send_poll(uint16_t addr, const uint8_t *buf, uint8_t len)
{
    return i2c::send_poll(addr, buf, len);
}

bool i2c2::send_recv_poll(uint16_t addr, const uint8_t *buf, uint8_t len, uint8_t *out, size_t max_out_len, size_t *actual_out_len)
{
    return i2c::send_recv_poll(addr, buf, len, out, max_out_len, actual_out_len);
}

void i2c2::recv(uint16_t addr, size_t expected_len)
{
    i2c::recv(addr, expected_len);
}

uint8_t *i2c2::begin_read_rx_buf(size_t *avail_len)
{
    return i2c::begin_read_rx_buf(avail_len);
}

void i2c2::done_read_rx_buf(size_t read_len)
{
    i2c::done_read_rx_buf(read_len);
}

size_t i2c2::get_rx_buf_len()
{
    return i2c::get_rx_buf_len();
}

void i2c2::handle_task()
{
    i2c::handle_task();
    if (recv_byte != -1) {
        volatile uint8_t byte_recved = recv_byte & 0xff;
        recv_byte = -1;
        auto *acq_buf = rx_buf.WriteAcquire(1);
        *acq_buf = byte_recved;
        rx_buf.WriteRelease(1);
    }
}


bool i2c3::init(bool fast_mode, bool enable_pull, i2c_def::timing_preset timing)
{
    i2c_periph = I2C3;
    LL_GPIO_InitTypeDef gpio_config = {};

    LL_RCC_SetI2CClockSource(LL_RCC_I2C2_CLKSOURCE_SYSCLK);
    if (!LL_AHB2_GRP1_IsEnabledClock(LL_AHB2_GRP1_PERIPH_GPIOC)) {
        LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);
    }

    gpio_config.Pin = LL_GPIO_PIN_0 | LL_GPIO_PIN_1;
    gpio_config.Mode = LL_GPIO_MODE_ALTERNATE;
    gpio_config.Speed = LL_GPIO_SPEED_FREQ_LOW;
    gpio_config.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
    gpio_config.Pull = enable_pull ? LL_GPIO_PULL_UP : LL_GPIO_PULL_NO;
    gpio_config.Alternate = LL_GPIO_AF_4;
    LL_GPIO_Init(GPIOC, &gpio_config);

    if (!LL_APB1_GRP1_IsEnabledClock(LL_APB1_GRP1_PERIPH_I2C3)) {
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C3);
    }

    NVIC_SetPriority(I2C2_EV_IRQn, 0);
    NVIC_EnableIRQ(I2C2_EV_IRQn);

    LL_I2C_EnableAutoEndMode(i2c_periph);
    LL_I2C_DisableOwnAddress2(i2c_periph);
    LL_I2C_DisableGeneralCall(i2c_periph);
    LL_I2C_EnableClockStretching(i2c_periph);

    LL_I2C_InitTypeDef i2c_config = {};
    i2c_config.PeripheralMode = LL_I2C_MODE_I2C; // I2C instead of SMBUS
    i2c_config.Timing = timing;
    i2c_config.AnalogFilter = LL_I2C_ANALOGFILTER_ENABLE;
    i2c_config.DigitalFilter = 0x00;
    i2c_config.OwnAddress1 = 0;
    i2c_config.TypeAcknowledge = LL_I2C_ACK;
    i2c_config.OwnAddrSize = LL_I2C_OWNADDRESS1_7BIT;

    LL_I2C_Init(i2c_periph, &i2c_config);
    LL_I2C_SetOwnAddress2(i2c_periph, 0, LL_I2C_OWNADDRESS2_NOMASK);

    return true;
}

bool i2c3::send_poll(uint16_t addr, const uint8_t *buf, uint8_t len)
{
    return i2c::send_poll(addr, buf, len);
}

bool i2c3::send_recv_poll(uint16_t addr, const uint8_t *buf, uint8_t len, uint8_t *out, size_t max_out_len, size_t *actual_out_len)
{
    return i2c::send_recv_poll(addr, buf, len, out, max_out_len, actual_out_len);
}

void i2c3::recv(uint16_t addr, size_t expected_len)
{
    i2c::recv(addr, expected_len);
}

uint8_t *i2c3::begin_read_rx_buf(size_t *avail_len)
{
    return i2c::begin_read_rx_buf(avail_len);
}

void i2c3::done_read_rx_buf(size_t read_len)
{
    i2c::done_read_rx_buf(read_len);
}

size_t i2c3::get_rx_buf_len()
{
    return i2c::get_rx_buf_len();
}

void i2c3::handle_task()
{
    i2c::handle_task();
    if (recv_byte != -1) {
        volatile uint8_t byte_recved = recv_byte & 0xff;
        recv_byte = -1;
        auto *acq_buf = rx_buf.WriteAcquire(1);
        *acq_buf = byte_recved;
        rx_buf.WriteRelease(1);
    }
}
