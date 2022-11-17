#include <cstring>
#include "uart_cmd.hpp"

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
