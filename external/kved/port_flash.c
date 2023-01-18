#include <stdint.h>
#include <stdbool.h>

#include "kved.h"
#include "kved_flash.h"

#include "flash_ll.h"

const uint32_t sector_size[KVED_FLASH_NUM_SECTORS] = { STM32WL_FLASH_SECTOR_SIZE, STM32WL_FLASH_SECTOR_SIZE };
const uint32_t sector_address[KVED_FLASH_NUM_SECTORS] = { 0x0803f000UL, 0x0803f800UL };
const uint8_t sector_page[KVED_FLASH_NUM_SECTORS] = { 126, 127 };

bool kved_flash_sector_erase(kved_flash_sector_t sec)
{
    if (sec >= KVED_FLASH_NUM_SECTORS) {
        return false;
    }

    return stm32wl_sector_erase(sector_page[sec]);
}

void kved_flash_data_write(kved_flash_sector_t sec, uint16_t index, kved_word_t data)
{
    uint32_t addr = sector_address[sec] + (index * sizeof(kved_word_t));
    stm32wl_flash_write(addr, data);
}

kved_word_t kved_flash_data_read(kved_flash_sector_t sec, uint16_t index)
{
    uint32_t addr = sector_address[sec] + (index * sizeof(kved_word_t));
    return (kved_word_t)stm32wl_flash_read_word(addr);
}

uint32_t kved_flash_sector_size()
{
    return STM32WL_FLASH_SECTOR_SIZE;
}

void kved_flash_init()
{
    // No-op
}
