#pragma once

#include <cstdint>
#include <cstddef>
#include "lfbb.hpp"

namespace i2c_def
{
    // Sourced from STM32CubeMX v6.7
    enum timing_preset : uint32_t {
        TIMING_FAST_48M = 0x2010091a, // Fast mode, 400KHz, I2C peripheral clock input @ 48MHz
        TIMING_FAST_16M = 0x0010061a, // Fast mode, 400KHz, I2C peripheral clock input @ 16MHz (e.g. direct from HSI)
        TIMING_STD_48M  = 0x20303e5d, // Standard mode, 100KHz, I2C peripheral clock input @ 48MHz
        TIMING_STD_16M  = 0x00303d5b, // Standard mode, 100KHz, I2C peripheral clock input @ 16MHz (e.g. direct from HSI)
    };
}

class i2c
{
public:
    virtual bool init(bool fast_mode, bool enable_pull, i2c_def::timing_preset timing) = 0;
    virtual bool send_poll(uint16_t addr, const uint8_t *buf, uint8_t len);
    virtual bool send_recv_poll(uint16_t addr, const uint8_t *buf, uint8_t len, uint8_t *out, size_t max_out_len, size_t *actual_out_len);
    virtual void recv(uint16_t addr, size_t expected_len);
    virtual uint8_t *begin_read_rx_buf(size_t *avail_len);
    virtual void done_read_rx_buf(size_t read_len);
    virtual size_t get_rx_buf_len();
    virtual void handle_task();

    void enable_10bit_mode(bool enable);

protected:
    bool addr_10bit = false;
    I2C_TypeDef *i2c_periph = nullptr;
    LfBb<uint8_t, 1024> rx_buf = {};
};

class i2c1 : public i2c
{
public:
    bool init(bool fast_mode, bool enable_pull, i2c_def::timing_preset timing) override;
    bool send_poll(uint16_t addr, const uint8_t *buf, uint8_t len) final;
    bool send_recv_poll(uint16_t addr, const uint8_t *buf, uint8_t len, uint8_t *out, size_t max_out_len, size_t *actual_out_len) final;
    void recv(uint16_t addr, size_t expected_len) final;
    uint8_t *begin_read_rx_buf(size_t *avail_len) final;
    void done_read_rx_buf(size_t read_len) final;
    size_t get_rx_buf_len() final;
    void handle_task() final;

public:
    static volatile bool bus_error;
    static volatile bool overrun_error;
    static volatile int32_t recv_byte;
};

class i2c2 : public i2c
{
public:
    bool init(bool fast_mode, bool enable_pull, i2c_def::timing_preset timing) override;
    bool send_poll(uint16_t addr, const uint8_t *buf, uint8_t len) final;
    bool send_recv_poll(uint16_t addr, const uint8_t *buf, uint8_t len, uint8_t *out, size_t max_out_len, size_t *actual_out_len) final;
    void recv(uint16_t addr, size_t expected_len) final;
    uint8_t *begin_read_rx_buf(size_t *avail_len) final;
    void done_read_rx_buf(size_t read_len) final;
    size_t get_rx_buf_len() final;
    void handle_task() final;

public:
    static volatile bool bus_error;
    static volatile bool overrun_error;
    static volatile int32_t recv_byte;

};

class i2c3 : public i2c
{
public:
    bool init(bool fast_mode, bool enable_pull, i2c_def::timing_preset timing) override;
    bool send_poll(uint16_t addr, const uint8_t *buf, uint8_t len) final;
    bool send_recv_poll(uint16_t addr, const uint8_t *buf, uint8_t len, uint8_t *out, size_t max_out_len, size_t *actual_out_len) final;
    void recv(uint16_t addr, size_t expected_len) final;
    uint8_t *begin_read_rx_buf(size_t *avail_len) final;
    void done_read_rx_buf(size_t read_len) final;
    size_t get_rx_buf_len() final;
    void handle_task() final;

public:
    static volatile bool bus_error;
    static volatile bool overrun_error;
    static volatile int32_t recv_byte;
};
