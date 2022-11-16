#pragma once

#include <uart.hpp>
#include "lpuart.hpp"

class uart_cmd final : public uart_rx_notifiable
{
public:
    static uart_cmd *instance()
    {
        static uart_cmd _instance;
        return &_instance;
    }

    uart_cmd(uart_cmd const &) = delete;
    void operator=(uart_cmd const &) = delete;

public:
    bool on_pkt_received() override;

private:
    uart_cmd() = default;

private:
    volatile bool decode_started = false;
    size_t decoded_len = 0;
    lpuart *uart = lpuart::instance();
    uint8_t decoded_buf[1024] = {};

    static const constexpr uint8_t SSLIP_START = 0xa5;
    static const constexpr uint8_t SSLIP_END = 0xc0;
    static const constexpr uint8_t SSLIP_ESC = 0xdb;
    static const constexpr uint8_t SSLIP_ESC_END = 0xdc;
    static const constexpr uint8_t SSLIP_ESC_ESC = 0xdd;
    static const constexpr uint8_t SSLIP_ESC_START = 0xde;
};

