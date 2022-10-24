#pragma once

#include <lfbb.h>
#include "uart.hpp"

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
    void on_pkt_recv(uint8_t *buf, size_t len) override;
    void set_tx(bool enable) override;
    void set_rx(bool enable) override;
    void handle_task() override; // In main loop
    void on_intr(); // In interrupt routine

private:
    volatile bool rx_avail = false;
    volatile bool tx_avail = false;
    volatile bool error = false;

private:
    LFBB_Inst_Type rx_fifo = {};
    LFBB_Inst_Type tx_fifo = {};
    static uint8_t rx_buf[512];
    static uint8_t tx_buf[512];
    lpuart() = default;
};
