#pragma once

#include <cstdint>
#include <cstddef>

class uart
{
public:
    virtual bool init() = 0;
    virtual bool transmit(uint8_t *buf, size_t len) = 0;
    virtual void on_pkt_recv(uint8_t *buf, size_t len) = 0;
    virtual void set_tx(bool enable) = 0;
    virtual void set_rx(bool enable) = 0;
    virtual void handle_task() = 0;
};