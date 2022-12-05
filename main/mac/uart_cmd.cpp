#include <cstring>
#include <stm32wlxx_ll_crc.h>
#include <stm32wlxx_ll_utils.h>
#include "stm32wlxx_ll_bus.h"
#include "uart_cmd.hpp"
#include "log.h"
#include "../misc.hpp"

bool uart_cmd::init()
{
    mac_addr = get_mac();

    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_CRC);
    LL_CRC_SetInputDataReverseMode(CRC, LL_CRC_INDATA_REVERSE_NONE);
    LL_CRC_SetOutputDataReverseMode(CRC, LL_CRC_OUTDATA_REVERSE_NONE);
    LL_CRC_SetInitialData(CRC, 0);

    return true;
}


bool uart_cmd::on_uart_pkt_recv()
{
    size_t len = uart->get_rx_buf_len();
    while (len > 0) {
        uint8_t *data = uart->begin_read_rx_buf(1, &len);
        uint8_t curr_byte = *data;
        uart->done_read_rx_buf(1);
        switch (curr_byte) {
            case SSLIP_START: {
                memset(decoded_buf, 0, sizeof(decoded_buf));
                decoded_len = 0;
                break;
            }

            case SSLIP_END: {
                handle_pkt();
                break; // Handle end of receive
            }

            case SSLIP_ESC: {
                data = uart->begin_read_rx_buf(1, &len);
                uint8_t next_byte = *data;
                uart->done_read_rx_buf(1);

                switch (next_byte) {
                    case SSLIP_ESC_END: {
                        decoded_buf[decoded_len] = SSLIP_END;
                        decoded_len += 1;
                        break;
                    }

                    case SSLIP_ESC_ESC: {
                        decoded_buf[decoded_len] = SSLIP_ESC;
                        decoded_len += 1;
                        break;
                    }

                    case SSLIP_ESC_START: {
                        decoded_buf[decoded_len] = SSLIP_START;
                        decoded_len += 1;
                        break;
                    }

                    default: {
                        send_nack_pkt();
                        break; // TODO: something screwed up - send NACK
                    }
                }

                break;
            }

            default: {
                decoded_buf[decoded_len] = curr_byte;
                decoded_len += 1;
                break;
            }
        }
    }

    return false;
}

void uart_cmd::encode_sslip_and_tx(cmd_def::uart_pkt_header *header, uint8_t *data, size_t len)
{
    const uint8_t esc_start[] = { SSLIP_ESC, SSLIP_ESC_START };
    const uint8_t esc_end[] = { SSLIP_ESC, SSLIP_ESC_END };
    const uint8_t esc_esc[] = { SSLIP_ESC, SSLIP_ESC_ESC };

    if (header == nullptr) {
        return;
    }

    auto *header_buf = (uint8_t *)header;

    uart->transmit((uint8_t *)&SSLIP_START, 1);
    for (size_t idx = 0; idx < sizeof(cmd_def::uart_pkt_header); idx += 1) {
        switch (header_buf[idx]) {
            case SSLIP_ESC: {
                uart->transmit((uint8_t *)(esc_esc), sizeof(esc_esc));
                break;
            }

            case SSLIP_START: {
                uart->transmit((uint8_t *)(esc_start), sizeof(esc_start));
                break;
            }

            case SSLIP_END: {
                uart->transmit((uint8_t *)(esc_end), sizeof(esc_end));
                break;
            }

            default: {
                uart->transmit(&data[idx], 1);
                break;
            }
        }
    }

    if (data != nullptr && len > 0) {
        for (size_t idx = 0; idx < len; idx += 1) {
            switch (data[idx]) {
                case SSLIP_ESC: {
                    uart->transmit((uint8_t *)(esc_esc), sizeof(esc_esc));
                    break;
                }

                case SSLIP_START: {
                    uart->transmit((uint8_t *)(esc_start), sizeof(esc_start));
                    break;
                }

                case SSLIP_END: {
                    uart->transmit((uint8_t *)(esc_end), sizeof(esc_end));
                    break;
                }

                default: {
                    uart->transmit(&data[idx], 1);
                    break;
                }
            }
        }
    }

    uart->transmit((uint8_t *)&SSLIP_END, 1);
}

bool uart_cmd::handle_pkt()
{
    auto *header = (cmd_def::uart_pkt_header *)(decoded_buf);
    uint16_t origin_crc = header->crc;
    header->crc = 0;
    uint16_t actual_crc = crc_16(decoded_buf, decoded_len);

    if (origin_crc != actual_crc) {
        WLB_LOG("CRC mismatched: 0x%04x vs 0x%04x", origin_crc, actual_crc);
        send_nack_pkt();
        return false;
    }

    switch (header->opcode) {
        case cmd_def::UART_OP_PING: {
            send_ack_pkt();
            break;
        }

        case cmd_def::UART_OP_DEVICE_INFO: {
            send_device_info();
            break;
        }

        case cmd_def::UART_OP_LORA_CFG: {
            if (handle_lora_cfg(decoded_buf + sizeof(cmd_def::uart_pkt_header), decoded_len - sizeof(cmd_def::uart_pkt_header))) {
                WLB_LOG("LoRa cfg ok");
                send_ack_pkt();
            } else {
                WLB_LOG("LoRa cfg error");
                send_nack_pkt();
            }

            break;
        }

        case cmd_def::UART_OP_SEND_LORA_PKT: {
            if (handle_lora_tx(decoded_buf + sizeof(cmd_def::uart_pkt_header), decoded_len - sizeof(cmd_def::uart_pkt_header))) {
                WLB_LOG("LoRa Tx ok");
                send_ack_pkt();
            } else {
                WLB_LOG("LoRa Tx error");
                send_nack_pkt();
            }

            break;
        }

        case cmd_def::UART_OP_RESET_RADIO: {
            break;
        }

        default: {
            WLB_LOG("Unknown opcode: 0x%02x\n", header->opcode);
            send_nack_pkt();
            break;
        }
    }

    return true;
}

bool uart_cmd::handle_lora_cfg(void *offset, size_t len)
{
    if (len < sizeof(cmd_def::uart_lora_cfg_pkt) || offset == nullptr) {
        WLB_LOG("LoRa cfg pkt len err: %u\n", len);
        return false;
    }

    auto *pkt = (cmd_def::uart_lora_cfg_pkt *)offset;
    WLB_LOG("LoRa cfg: BW=%u @ SF=%u, CR=%u; Freq=%lu; LDRO=%u; sw=0x%x", pkt->bw, pkt->sf, pkt->cr, pkt->freq_hz, pkt->ldro, pkt->sync_word);

    return lora->setup_lora(pkt->freq_hz, (sx126x_lora_sf_t)pkt->sf, (sx126x_lora_bw_t)pkt->bw, pkt->ldro != 0, (sx126x_lora_cr_t)pkt->cr, pkt->sync_word);;
}

bool uart_cmd::handle_lora_tx(void *offset, size_t len)
{
    if (len < (sizeof(cmd_def::uart_tx_pkt) - sizeof(cmd_def::uart_tx_pkt::buf)) || offset == nullptr) {
        WLB_LOG("LoRa tx pkt len err: %u\n", len);
        return false;
    }

    auto *pkt = (cmd_def::uart_tx_pkt *)offset;
    WLB_LOG("LoRa Tx len=%u @ PWR=%d; timeout=%lu, preamble=%u; header=%u, crc=%u, invert_iq=%u",
            pkt->len, pkt->tx_pwr, pkt->timeout_ms, pkt->preamble_cnt, pkt->header_en, pkt->crc_on, pkt->invert_iq);

    return lora->set_lora_tx(pkt->buf, pkt->len, pkt->tx_pwr, pkt->timeout_ms, pkt->preamble_cnt,
                             pkt->header_en != 0, pkt->crc_on != 0, pkt->invert_iq != 0);
}

void uart_cmd::send_ack_pkt()
{
    cmd_def::uart_pkt_header header = {};
    header.opcode = cmd_def::UART_OP_ACK;
    header.crc = 0;

    uint16_t actual_crc = crc_16((uint8_t *)&header, sizeof(header));
    header.crc = actual_crc;

    encode_sslip_and_tx(&header, nullptr, 0);
}

void uart_cmd::send_nack_pkt()
{
    cmd_def::uart_pkt_header header = {};
    header.opcode = cmd_def::UART_OP_NACK;
    header.crc = 0;

    uint16_t actual_crc = crc_16((uint8_t *)&header, sizeof(header));
    header.crc = actual_crc;

    encode_sslip_and_tx(&header, nullptr, 0);
}

void uart_cmd::send_device_info()
{
    cmd_def::uart_pkt_header header = {};
    header.opcode = cmd_def::UART_OP_DEVICE_INFO;
    header.crc = 0;

    uint16_t actual_crc = crc_16((uint8_t *)&header, sizeof(header));

    cmd_def::uart_dev_info_pkt pkt = {};
    pkt.mac = mac_addr;

    uint32_t uid0 = LL_GetUID_Word0();
    uint32_t uid1 = LL_GetUID_Word1();
    uint32_t uid2 = LL_GetUID_Word2();

    memcpy(&pkt.uid[0], &uid0, sizeof(uint32_t));
    memcpy(&pkt.uid[4], &uid1, sizeof(uint32_t));
    memcpy(&pkt.uid[8], &uid2, sizeof(uint32_t));

    pkt.fw_ver = WLB_FW_VER;

    actual_crc = crc_16((uint8_t *)&pkt, sizeof(pkt), actual_crc);
    header.crc = actual_crc;

    encode_sslip_and_tx(&header, (uint8_t *)&pkt, sizeof(pkt));
}

uint64_t uart_cmd::get_mac()
{
    auto dev_id = (uint32_t)(READ_REG(*((uint32_t *)(UID64_BASE))));
    auto maker_id = (uint32_t)(READ_REG(*((uint32_t *)(UID64_BASE + 4u))));
    return (uint64_t)(((uint64_t)dev_id << 32u) | maker_id);
}

uint16_t uart_cmd::crc_16(uint8_t *buf, size_t len, uint16_t init, uint16_t poly)
{
    size_t idx = 0;
    LL_CRC_ResetCRCCalculationUnit(CRC);
    LL_CRC_SetPolynomialSize(CRC, LL_CRC_POLYLENGTH_16B);
    LL_CRC_SetPolynomialCoef(CRC, poly);
    LL_CRC_SetInitialData(CRC, init);

    for (idx = 0; idx < (len / 4); idx += 1) {
        auto data = (uint32_t)((uint32_t)(buf[idx] << 24) | (uint32_t)(buf[idx] << 16) | (uint32_t)(buf[idx] << 8) | (uint32_t)(buf[idx]));
        LL_CRC_FeedData32(CRC, data);
    }

    if ((len % 4) != 0) {
        if (len % 4 == 1) {
            LL_CRC_FeedData8(CRC, buf[4 * idx]);
        }

        if (len % 4 == 2) {
            LL_CRC_FeedData16(CRC, (uint16_t)((buf[4 * idx + 1] << 8) | buf[4 * idx]));
        }

        if (len % 4 == 3) {
            LL_CRC_FeedData16(CRC, (uint16_t)((buf[4 * idx + 1] << 8) | buf[4 * idx]));
            LL_CRC_FeedData8(CRC, buf[4 * idx + 2]);
        }
    }

    return LL_CRC_ReadData16(CRC);
}

void uart_cmd::on_subghz_tx_done()
{
    cmd_def::uart_pkt_header header = {};
    header.opcode = cmd_def::UART_OP_SEND_LORA_PKT;
    header.crc = 0;

    uint16_t actual_crc = crc_16((uint8_t *)&header, sizeof(header));
    header.crc = actual_crc;

    encode_sslip_and_tx(&header, nullptr, 0);
}

void uart_cmd::on_subghz_rx_done()
{
    cmd_def::uart_pkt_header header = {};
    header.opcode = cmd_def::UART_OP_RECV_LORA_PKT;
    header.crc = 0;

    uint16_t actual_crc = crc_16((uint8_t *)&header, sizeof(header));
    header.crc = actual_crc;

    cmd_def::uart_rx_pkt pkt = {};
    sx126x_pkt_status_lora_t pkt_status = {};

    if (!lora->get_lora_pkt_status(&pkt_status)) {
        // Send internal error here!
        return;
    }

    pkt.pkt_rssi = (uint8_t)abs(pkt_status.rssi_pkt_in_dbm);
    pkt.sig_rssi = (uint8_t)abs(pkt_status.signal_rssi_pkt_in_dbm);
    pkt.snr = pkt_status.snr_pkt_in_db;

    if (!lora->read_rx_buf(pkt.buf, sizeof(cmd_def::uart_rx_pkt::buf), &pkt.len)) {
        // Send internal error here!
        return;
    }

    actual_crc = crc_16((uint8_t *)&pkt, sizeof(pkt), actual_crc);
    header.crc = actual_crc;

    encode_sslip_and_tx(&header, nullptr, 0);
}

void uart_cmd::on_subghz_timeout()
{

}

void uart_cmd::on_subghz_crc_error()
{

}

void uart_cmd::on_subghz_header_error()
{

}
