#pragma once

#include <stdbool.h>
#include "sx126x_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

bool stm32wl_subghz_init();
bool stm32wl_subghz_deinit();

#ifdef __cplusplus
}
#endif
