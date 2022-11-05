#pragma once

#include <cstdint>
#include "subghz/sx126x.h"

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

public:
    static volatile sx126x_irq_mask_t last_irq_status;
};
