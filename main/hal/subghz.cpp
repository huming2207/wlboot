#include <stm32wlxx_it.h>
#include "subghz.hpp"

volatile sx126x_irq_mask_t subghz::last_irq_status = 0;

#include "subghz/sx126x.h"

extern "C" void SUBGHZ_Radio_IRQHandler()
{
    sx126x_irq_mask_t irq_status = 0;
    if (sx126x_get_and_clear_irq_status(nullptr, &irq_status) == SX126X_STATUS_OK) {
        subghz::last_irq_status = irq_status;
    }
}