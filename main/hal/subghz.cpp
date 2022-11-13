#include <stm32wlxx_it.h>
#include "subghz.hpp"

#include "subghz/sx126x.h"
#include "subghz/stm32wl_subghz_hal.h"
#include "log.h"

extern "C" void SUBGHZ_Radio_IRQHandler()
{
    sx126x_irq_mask_t irq_status = 0;
    if (sx126x_get_and_clear_irq_status(nullptr, &irq_status) == SX126X_STATUS_OK) {
        subghz::last_irq_status = irq_status;
    }
}

volatile sx126x_irq_mask_t subghz::last_irq_status = 0;

bool subghz::init()
{
    if (!stm32wl_subghz_init()) {
        WLB_LOG("SUBGHZ init fail\n");
        return false;
    }

    if (sx126x_set_sleep(nullptr, SX126X_SLEEP_CFG_COLD_START) != SX126X_STATUS_OK) {
        WLB_LOG("SUBGHZ sleep fail\n");
    }

    WLB_LOG("SUBGHZ init OK\n");
    return true;
}

bool subghz::setup_lora(uint32_t freq_hz, sx126x_lora_sf_t sf, sx126x_lora_bw_t bw, bool low_data_rate_opt, sx126x_lora_cr_t cr, uint8_t sync_word, uint16_t img_cal_start_mhz, uint16_t img_cal_end_mhz)
{
    // Step 1. Go standby
    if (pwr_mode != lora::STDBY_RC) {
        if (sx126x_set_standby(nullptr, SX126X_STANDBY_CFG_RC) != SX126X_STATUS_OK) {
            WLB_LOG("SUBGHZ set standby fail\n");
            return false;
        } else {
            pwr_mode = lora::STDBY_RC;
        }
    }

    // Step 2. Packet type = LoRa
    auto ret = sx126x_set_pkt_type(nullptr, SX126X_PKT_TYPE_LORA);

    // Step 3. Recalibration
    ret = static_cast<sx126x_status_t>(ret ? ret : sx126x_cal(nullptr, SX126X_CAL_ALL));

    // Step 4. Set RF frequency & do image calibration
    ret = static_cast<sx126x_status_t>(ret ? ret : sx126x_set_rf_freq(nullptr, freq_hz));
    ret = static_cast<sx126x_status_t>(ret ? ret : sx126x_cal_img_in_mhz(nullptr, img_cal_start_mhz, img_cal_end_mhz));

    // Step 5: Buffer address (override to 0 for now?)
    ret = static_cast<sx126x_status_t>(ret ? ret : sx126x_set_buffer_base_address(nullptr, 0, 0));

    // Step 6: Set modulation parameters
    sx126x_mod_params_lora_t mod_params = {};
    mod_params.bw = bw;
    mod_params.cr = cr;
    mod_params.sf = sf;
    mod_params.ldro = low_data_rate_opt ? 1 : 0;

    ret = static_cast<sx126x_status_t>(ret ? ret : sx126x_set_lora_mod_params(nullptr, &mod_params));

    // Step 7: Set sync word
    ret = static_cast<sx126x_status_t>(ret ? ret : sx126x_set_lora_sync_word(nullptr, sync_word));
    return ret == SX126X_STATUS_OK;
}

void subghz::handle_task()
{
    volatile sx126x_irq_mask_t irq_status = last_irq_status;
    last_irq_status = 0;

    if (irq_status & SX126X_IRQ_RX_DONE) {

    }
}

bool subghz::set_lora_tx(uint8_t *buf, uint8_t len, int8_t tx_power, uint32_t timeout_ms, uint16_t preamble_cnt, bool header_en, bool crc_on, bool invert_iq)
{
    auto ret = SX126X_STATUS_OK;
    auto tx_pwr_level = (int8_t)(tx_power > 14 ? tx_power : 14);
    for (uint32_t idx = 0; idx < (sizeof(pa_cfg_lut) / sizeof(lora::pa_cfg_lut_item)); idx += 1) {
        if (pa_cfg_lut[idx].tx_pwr == tx_pwr_level) {
            ret = static_cast<sx126x_status_t>(ret ? ret : sx126x_set_pa_cfg(nullptr, &pa_cfg_lut[idx].pa_cfg));
        }
    }

    ret = static_cast<sx126x_status_t>(ret ? ret : sx126x_set_tx_params(nullptr, tx_power, SX126X_RAMP_3400_US)); // RampTime = 0x7 - need review

    sx126x_pkt_params_lora_t pkt_params = {};
    pkt_params.crc_is_on = crc_on;
    pkt_params.invert_iq_is_on = invert_iq;
    pkt_params.header_type = header_en ? SX126X_LORA_PKT_EXPLICIT : SX126X_LORA_PKT_IMPLICIT;
    pkt_params.preamble_len_in_symb = preamble_cnt;
    pkt_params.pld_len_in_bytes = len;
    ret = static_cast<sx126x_status_t>(ret ? ret : sx126x_set_lora_pkt_params(nullptr, &pkt_params));

    uint16_t dio_masks = SX126X_IRQ_TX_DONE | SX126X_IRQ_CRC_ERROR | SX126X_IRQ_HEADER_ERROR | SX126X_IRQ_TIMEOUT;
    ret = static_cast<sx126x_status_t>(ret ? ret : sx126x_set_dio_irq_params(nullptr, dio_masks, dio_masks, 0, 0));

    ret = static_cast<sx126x_status_t>(ret ? ret : sx126x_write_buffer(nullptr, 0, buf, len));

    ret = static_cast<sx126x_status_t>(ret ? ret : sx126x_set_tx(nullptr, timeout_ms));
    pwr_mode = lora::TX;
    return ret == SX126X_STATUS_OK;
}

