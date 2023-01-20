#include <cstdio>
#include "subghz.hpp"
#include "log.h"
#include "kved.h"
#include "flash_ll.h"
#include "clock_setup.h"

#include <lpuart.hpp>

#ifndef DISABLE_LOG
extern "C" void initialise_monitor_handles(void);
#endif

int main()
{
#ifndef DISABLE_LOG
    initialise_monitor_handles();
#endif

    clock_init();
    auto *uart = lpuart::instance();
    auto *lora= subghz::instance();

    uart->init();
    lora->init();

    if (!lora->setup_lora(918000000, SX126X_LORA_SF7, SX126X_LORA_BW_125, false)) {
        WLB_LOG("LoRa mode init failed");
    }

    uint8_t buf[8] = { 0xca, 0xfe, 0xbe, 0xef, 0x5a, 0xa5, 0xaa, 0x55 };
    while (true) {
        uart->handle_task();
        lora->handle_task();
        lora->set_lora_tx(buf, sizeof(buf), 20, 1000, 20, true);
        LL_mDelay(300);
    }

    uint8_t rx_buf[8] = {};
    while (true) {
        uart->handle_task();
        lora->handle_task();
        lora->set_lora_rx(sizeof(rx_buf), 1000, 20, true, true);
        LL_mDelay(1000);
    }
}

