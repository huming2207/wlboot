#pragma once

#include <cstdint>
#include "subghz/sx126x.h"

namespace lora
{
    enum pwr_mode : int8_t
    {
        SLEEP = -1,
        STDBY_RC = 0,
        STDBY_XOSC = 1,
        FS = 2,
        RX = 3,
        TX = 4,
    };

    struct pa_cfg_lut_item
    {
        sx126x_pa_cfg_params_t pa_cfg;
        int8_t tx_pwr;
    };
}

class subghz
{
public:
    static subghz *instance()
    {
        static subghz _instance;
        return &_instance;
    }

    subghz(subghz const &) = delete;
    void operator=(subghz const &) = delete;

private:
    subghz() = default;

public:
    bool init();
    bool setup_lora(uint32_t freq_hz, sx126x_lora_sf_t sf, sx126x_lora_bw_t bw, bool low_data_rate_opt,
                    sx126x_lora_cr_t cr = SX126X_LORA_CR_4_5, uint8_t sync_word = 0x68, uint16_t img_cal_start_mhz = 915, uint16_t img_cal_end_mhz = 928);
    bool set_lora_tx(uint8_t *buf, uint8_t len, int8_t tx_power, uint32_t timeout_ms, uint16_t preamble_cnt, bool header_en = true, bool crc_on = true, bool invert_iq = false);
    bool set_lora_rx(uint8_t len, uint32_t timeout_ms, uint16_t preamble_cnt, bool rx_boost = true, bool header_en = true, bool crc_on = true, bool invert_iq = false);
    bool read_rx_buf(uint8_t *buf, uint8_t len, uint8_t *actual_len);
    void handle_task();

public:
    volatile lora::pwr_mode pwr_mode = lora::SLEEP;
    static volatile sx126x_irq_mask_t last_irq_status;
    static const constexpr lora::pa_cfg_lut_item pa_cfg_lut[] =
    {
            {{0x04, 0x07, 0x00, 0x01}, 22},
            {{0x04, 0x07, 0x00, 0x01}, 21}, // Probably can re-tune??
            {{0x03, 0x05, 0x00, 0x01}, 20},
            {{0x03, 0x05, 0x00, 0x01}, 19}, // Probably can re-tune??
            {{0x03, 0x05, 0x00, 0x01}, 18}, // Probably can re-tune??
            {{0x02, 0x03, 0x00, 0x01}, 17},
            {{0x02, 0x03, 0x00, 0x01}, 16}, // Probably can re-tune??
            {{0x02, 0x03, 0x00, 0x01}, 15}, // Probably can re-tune??
            {{0x02, 0x02, 0x00, 0x01}, 14},
    };
};
