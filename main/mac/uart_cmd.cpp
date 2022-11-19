#include <cstring>
#include <stm32wlxx_ll_crc.h>
#include <stm32wlxx_ll_utils.h>
#include "uart_cmd.hpp"
#include "log.h"

bool uart_cmd::init()
{
    mac_addr = get_mac();

    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_CRC);
    LL_CRC_SetInputDataReverseMode(CRC, LL_CRC_INDATA_REVERSE_NONE);
    LL_CRC_SetOutputDataReverseMode(CRC, LL_CRC_OUTDATA_REVERSE_NONE);
    LL_CRC_SetPolynomialSize(CRC, LL_CRC_POLYLENGTH_16B);
    LL_CRC_SetPolynomialCoef(CRC, CRC_POLY);
    LL_CRC_SetInitialData(CRC, 0);

    return true;
}


bool uart_cmd::on_pkt_received()
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
            break;
        }

        case cmd_def::UART_OP_LORA_CFG: {
            break;
        }

        case cmd_def::UART_OP_SEND_LORA_PKT: {
            break;
        }

        case cmd_def::UART_OP_RESET_RADIO: {
            break;
        }

        default: {
            WLB_LOG("Unknown opcode: 0x%02x", header->opcode);
            send_nack_pkt();
            break;
        }
    }

    return true;
}


void uart_cmd::send_ack_pkt()
{
    cmd_def::uart_pkt_header header = {};
    header.opcode = cmd_def::UART_OP_ACK;
    header.mac = mac_addr;
    header.crc = 0;

    uint16_t actual_crc = crc_16((uint8_t *)&header, sizeof(header));
    header.crc = actual_crc;

    uart->transmit((uint8_t *)&header, sizeof(header));
}

void uart_cmd::send_nack_pkt()
{
    cmd_def::uart_pkt_header header = {};
    header.opcode = cmd_def::UART_OP_NACK;
    header.mac = mac_addr;
    header.crc = 0;

    uint16_t actual_crc = crc_16((uint8_t *)&header, sizeof(header));
    header.crc = actual_crc;

    uart->transmit((uint8_t *)&header, sizeof(header));
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
