#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stm32wlxx_ll_system.h>

#define FLASH_RAM_FUNC __attribute__ ((long_call, section (".code_in_ram")))
#define STM32WL_FLASH_SECTOR_SIZE    2048UL
#define STM32WL_FLASH_WORD_SIZE      8UL

void stm32wl_flash_unlock();
void stm32wl_flash_lock();
void stm32wl_icache_disable();
void stm32wl_icache_enable();
void stm32wl_dcache_disable();
void stm32wl_dcache_enable();
void stm32wl_flash_wait();
bool stm32wl_check_error();
bool stm32wl_sector_erase(uint8_t idx);
bool stm32wl_flash_write(uint32_t addr, uint64_t data);
uint64_t stm32wl_flash_read_word(uint32_t addr);

#ifdef __cplusplus
}
#endif