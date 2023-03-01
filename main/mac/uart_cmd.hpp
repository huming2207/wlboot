#pragma once

#include <uart.hpp>
#include "lpuart.hpp"
#include "subghz.hpp"

namespace cmd_def
{
    static constexpr const uint32_t DEFAULT_FW_VER = 1000;
    enum uart_opcode : uint8_t
    {
        UART_OP_ACK = 0x00,

        // Error
        UART_OP_RADIO_TIMEOUT = 0xe0,
        UART_OP_RADIO_HEADER_ERROR = 0xe1,
        UART_OP_RADIO_CRC_ERROR = 0xe2,
        UART_OP_NACK = 0xff,
        UART_OP_ERR_INTERNAL = 0xfe,

        // Device setup
        UART_OP_PING = 0x01, // From host
        UART_OP_DEVICE_INFO = 0x02, // To host
        UART_OP_RADIO_RESET = 0x03,
        UART_OP_SYS_RESET = 0x04,
        UART_OP_LORA_CFG = 0x05,
        UART_OP_LORA_ADV_CFG = 0x06,

        // LoRa stuff
        UART_OP_SEND_LORA_PKT = 0x10, // From host -> SUBGHZ -> Air
        UART_OP_RECV_LORA_PKT = 0x11, // To host
    };

    struct __attribute__((packed)) uart_pkt_header
    {
        uart_opcode opcode;
        uint8_t ctr;
        uint16_t crc;
    };

    struct __attribute__((packed)) uart_ping_pkt
    {
        uint32_t ts;
    };

    struct __attribute__((packed)) uart_dev_info_pkt
    {
        uint32_t fw_ver;
        uint64_t mac;
        uint8_t uid[12];
    };

    struct __attribute__((packed)) uart_lora_cfg_pkt
    {
        uint8_t sf;
        uint8_t bw;
        uint8_t cr;
        uint8_t ldro;
        uint8_t sync_word;
        uint32_t freq_hz;
    };

    struct __attribute__((packed)) uart_tx_pkt
    {
        int8_t tx_pwr;
        uint32_t timeout_ms;
        uint16_t preamble_cnt;
        uint8_t header_en;
        uint8_t crc_on;
        uint8_t invert_iq;
        uint8_t len;
        uint8_t buf[255];
    };

    struct __attribute__((packed)) uart_rx_pkt
    {
        uint8_t pkt_rssi;
        uint8_t sig_rssi;
        uint8_t snr;
        uint8_t len;
        uint8_t buf[255];
    };
}

class uart_cmd final : private uart_rx_notifiable, private subghz_irq_notifiable
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
    bool init();
    bool handle_pkt();
    bool handle_lora_cfg(void *offset, size_t len);
    bool handle_lora_tx(void *offset, size_t len);
    void encode_sslip_and_tx(cmd_def::uart_pkt_header *header, uint8_t *data, size_t len);
    void send_ack_pkt();
    void send_nack_pkt();
    void send_device_info();

private:
    uart_cmd() = default;
    static uint64_t get_mac();
    static uint16_t crc_16(uint8_t *buf, size_t len, uint16_t init = 0, uint16_t poly = 0x1021);

    bool on_uart_pkt_recv() override;
    void on_subghz_tx_done() override;
    void on_subghz_rx_done() override;
    void on_subghz_timeout() override;
    void on_subghz_crc_error() override;
    void on_subghz_header_error() override;

private:
    volatile bool decode_started = false;
    volatile uint8_t curr_ctr = 0;

    size_t decoded_len = 0;
    lpuart *uart = lpuart::instance();
    subghz *lora = subghz::instance();
    uint64_t mac_addr = 0;
    static uint8_t decoded_buf[1024];

    static const constexpr uint8_t SSLIP_START = 0xa5;
    static const constexpr uint8_t SSLIP_END = 0xc0;
    static const constexpr uint8_t SSLIP_ESC = 0xdb;
    static const constexpr uint8_t SSLIP_ESC_END = 0xdc;
    static const constexpr uint8_t SSLIP_ESC_ESC = 0xdd;
    static const constexpr uint8_t SSLIP_ESC_START = 0xde;
};

