#pragma once

#include "uart.hpp"
#include "lfbb.hpp"

class lpuart : public uart
{
public:
    static lpuart *instance()
    {
        static lpuart _instance;
        return &_instance;
    }

    lpuart(lpuart const &) = delete;
    void operator=(lpuart const &) = delete;

public:
    bool init() override;
    bool transmit(uint8_t *buf, size_t len) override;
    uint8_t *begin_read_rx_buf(size_t buf_len, size_t *avail_len) override;
    void done_read_rx_buf(size_t len) override;
    void set_tx(bool enable) override;
    void set_rx(bool enable) override;
    size_t get_rx_buf_len() override;
    void set_rx_notify_byte(uint8_t byte, uart_rx_notifiable *handler) override;
    void handle_task() override; // In main loop

public:
    static volatile int32_t recv_byte;
    static volatile bool framing_error;
    static volatile bool noise_error;
    static volatile bool overrun_error;
    static volatile bool parity_error;

private:
    static LfBb<uint8_t, 1024> rx_buf;
    lpuart() = default;
    uint8_t rx_notify_byte = 0xff;
    uart_rx_notifiable *rx_notify_handler = nullptr;
};
