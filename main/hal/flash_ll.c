#include "flash_ll.h"
#include "log.h"

#define STM32WL_FLASH_KEY1      0x45670123UL
#define STM32WL_FLASH_KEY2      0xCDEF89ABUL

void stm32wl_flash_unlock()
{
    if (READ_BIT(FLASH->CR, FLASH_CR_LOCK) != 0) {
        WRITE_REG(FLASH->KEYR, STM32WL_FLASH_KEY1);
        WRITE_REG(FLASH->KEYR, STM32WL_FLASH_KEY2);
    }
}

FLASH_RAM_FUNC void stm32wl_flash_lock()
{
    SET_BIT(FLASH->CR, FLASH_CR_LOCK);
}

FLASH_RAM_FUNC void stm32wl_icache_disable()
{
    LL_FLASH_DisableInstCache();
}

FLASH_RAM_FUNC void stm32wl_icache_enable()
{
    LL_FLASH_EnableInstCache();
}

FLASH_RAM_FUNC void stm32wl_dcache_disable()
{
    LL_FLASH_DisableDataCache();
}

FLASH_RAM_FUNC void stm32wl_dcache_enable()
{
    LL_FLASH_EnableDataCache();
}

FLASH_RAM_FUNC void stm32wl_flash_wait()
{
    while(READ_BIT(FLASH->SR, FLASH_SR_BSY) != 0) {}
}

FLASH_RAM_FUNC bool stm32wl_flash_check_error()
{
    volatile bool has_err = false;

    if(READ_BIT(FLASH->SR, FLASH_SR_PROGERR) != 0) {
        SET_BIT(FLASH->SR, FLASH_SR_PROGERR);
        has_err = true;
    }

    if(READ_BIT(FLASH->SR, FLASH_SR_WRPERR) != 0) {
        SET_BIT(FLASH->SR, FLASH_SR_WRPERR);
        has_err = true;
    }

    if(READ_BIT(FLASH->SR, FLASH_SR_PGAERR) != 0) {
        SET_BIT(FLASH->SR, FLASH_SR_PGAERR);
        has_err = true;
    }

    if(READ_BIT(FLASH->SR, FLASH_SR_SIZERR) != 0) {
        SET_BIT(FLASH->SR, FLASH_SR_SIZERR);
        has_err = true;
    }

    if(READ_BIT(FLASH->SR, FLASH_SR_PGSERR) != 0) {
        SET_BIT(FLASH->SR, FLASH_SR_PGSERR);
        has_err = true;
    }

    if(READ_BIT(FLASH->SR, FLASH_SR_MISERR) != 0) {
        SET_BIT(FLASH->SR, FLASH_SR_MISERR);
        has_err = true;
    }

    if(READ_BIT(FLASH->SR, FLASH_SR_FASTERR) != 0) {
        SET_BIT(FLASH->SR, FLASH_SR_FASTERR);
        has_err = true;
    }

    if (READ_BIT(FLASH->SR, FLASH_SR_RDERR) != 0) {
        SET_BIT(FLASH->SR, FLASH_SR_RDERR);
        has_err = true;
    }

    return has_err;
}

FLASH_RAM_FUNC bool stm32wl_flash_sector_erase(uint8_t idx)
{
    // See RM0461 Section 3.3.7 and KVED reference code
    // 1. Wait first
    stm32wl_flash_wait();

    // 2. Check if there's any suspend ops
    if (READ_BIT(FLASH->SR, FLASH_SR_PESD) != 0) {
        return false;
    }

    // 3. Unlock, disable I/D-Cache
    stm32wl_flash_unlock();
    stm32wl_icache_disable();
    stm32wl_dcache_disable();

    // 4. Wait and check error
    stm32wl_flash_wait();
    if (stm32wl_flash_check_error()) {
        return false;
    }

    // 5. Set the page that needs to be erased
    MODIFY_REG(FLASH->CR, FLASH_CR_PNB, ((idx & 0xFFU) << FLASH_CR_PNB_Pos));

    // 6. Enable page erase
    SET_BIT(FLASH->CR, FLASH_CR_PER);

    // 7. Nuke the page
    SET_BIT(FLASH->CR, FLASH_CR_STRT);

    // 8. Wait for the nuke
    stm32wl_flash_wait();

    // 8. Clear page erase request
    CLEAR_BIT(FLASH->CR, (FLASH_CR_PER | FLASH_CR_PNB));

    // 9. Put back I/D-cache and flash lock
    stm32wl_icache_enable();
    stm32wl_dcache_enable();
    stm32wl_flash_lock();

    return true;
}

FLASH_RAM_FUNC bool stm32wl_flash_write(uint32_t addr, uint64_t data)
{
    // See RM0461 Section 3.3.8 and KVED reference code
    // 1. Wait first
    stm32wl_flash_wait();

    // 2. Check if there's any suspend ops
    if (READ_BIT(FLASH->SR, FLASH_SR_PESD) != 0) {
        return false;
    }

    // 3. Unlock, disable I/D-Cache
    stm32wl_flash_unlock();
    stm32wl_icache_disable();
    stm32wl_dcache_disable();

    // 4. Wait and check error
    stm32wl_flash_wait();
    if (stm32wl_flash_check_error()) {
        return false;
    }

    // 5. Enable programming
    SET_BIT(FLASH->CR, FLASH_CR_PG);

    // 6. Write the stuff
    *(volatile uint32_t*)addr = (uint32_t)data;
    __ISB(); // Instruction Synchronization Barrier - make sure the program is in order
    *(volatile uint32_t*)(addr + 4U) = (uint32_t)(data >> 32ull);

    // 7. Wait
    stm32wl_flash_wait();

    // 8. Check EOP
    bool success = false;
    if (READ_BIT(FLASH->SR, FLASH_SR_EOP) != 0) {
        SET_BIT(FLASH->SR, FLASH_SR_EOP);
        success = true;
    } else {
        success = false;
    }

    CLEAR_BIT(FLASH->CR, FLASH_CR_PG);
    stm32wl_icache_enable();
    stm32wl_dcache_enable();
    stm32wl_flash_lock();

    return success;
}

FLASH_RAM_FUNC uint64_t stm32wl_flash_read_word(uint32_t addr)
{
    return *((uint64_t *)addr);
}

FLASH_RAM_FUNC bool stm32wl_flash_write_buf(uint32_t addr, uint8_t *buf, uint32_t size)
{
    stm32wl_flash_wait();

    // Check suspend operations
    if (READ_BIT(FLASH->SR, FLASH_SR_PESD) != 0) {
        return false;
    }

    stm32wl_flash_unlock();
    stm32wl_icache_disable();
    stm32wl_dcache_disable();

    stm32wl_flash_wait();
    if (stm32wl_flash_check_error()) {
        return false;
    }

    // Enable programming
    SET_BIT(FLASH->CR, FLASH_CR_PG);

    volatile uint32_t buf_left = size;
    volatile uint32_t curr_addr = addr;
    volatile uint8_t *buf_ptr = buf;
    while (buf_left > 0) {
        if (buf_left >= 8) {
            stm32wl_flash_wait();

            *(volatile uint32_t*)curr_addr = *((uint32_t *)buf_ptr);
            __ISB(); // Make sure the program operation is in correct order
            *(volatile uint32_t*)(curr_addr + 4U) = *((uint32_t *)(buf_ptr + 4));

            curr_addr += 8;
            buf_ptr += 8;
            buf_left -= 8;

            stm32wl_flash_wait();
        } else {
            volatile uint8_t round_buf[8] = {};
            for (uint32_t idx = 0; idx < buf_left; idx += 1) {
                round_buf[idx] = *((uint8_t *)buf_ptr);
                buf_ptr += 1;
            }

            for (uint32_t idx = 0; idx < (8 - buf_left); idx += 1) {
                round_buf[idx + buf_left] = 0xff; // Fill 0xff for unused bytes
            }

            stm32wl_flash_wait();

            *(volatile uint32_t*)curr_addr = *((uint32_t *)round_buf);
            __ISB(); // Make sure the program operation is in correct order
            *(volatile uint32_t*)(curr_addr + 4U) = *((uint32_t *)(round_buf + 4));

            stm32wl_flash_wait();
            buf_left = 0;
        }
    }

    stm32wl_flash_wait();

    // End of program
    bool success = false;
    if (READ_BIT(FLASH->SR, FLASH_SR_EOP) != 0) {
        SET_BIT(FLASH->SR, FLASH_SR_EOP);
        success = true;
    } else {
        success = false;
    }

    CLEAR_BIT(FLASH->CR, FLASH_CR_PG);
    stm32wl_icache_enable();
    stm32wl_dcache_enable();
    stm32wl_flash_lock();

    return success;
}
