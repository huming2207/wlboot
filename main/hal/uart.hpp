#pragma once

#include <cstdint>
#include <cstddef>

class uart_rx_notifiable
{
public:
    virtual bool on_uart_pkt_recv() = 0;
};

class uart
{
public:
    virtual bool init() = 0;
    virtual bool transmit(uint8_t *buf, size_t len) = 0;
    virtual uint8_t *begin_read_rx_buf(size_t buf_len, size_t *avail_len) = 0;
    virtual void done_read_rx_buf(size_t read_len) = 0;
    virtual size_t get_rx_buf_len() = 0;
    virtual void set_tx(bool enable) = 0;
    virtual void set_rx(bool enable) = 0;
    virtual void set_rx_notify_byte(uint8_t byte, uart_rx_notifiable *handler) = 0;
    virtual void handle_task() = 0;
};

