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
    void handle_task() override; // In main loop
    void on_intr(); // In interrupt routine

private:
    volatile bool rx_avail = false;
    volatile bool noise_error = false;
    volatile bool parity_error = false;
    volatile bool overrun_error = false;
    volatile bool framing_error = false;

private:
    static LfBb<uint8_t, 1024> rx_buf;
    lpuart() = default;
};
