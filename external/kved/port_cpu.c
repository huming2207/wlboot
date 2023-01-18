#include <stdint.h>
#include <stm32wlxx_ll_system.h>

#include "kved_cpu.h"

void kved_cpu_critical_section_enter(void)
{
    __disable_irq();
}

void kved_cpu_critical_section_leave(void)
{
    __enable_irq();
}