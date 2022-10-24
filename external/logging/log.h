#pragma once

#include "printf/printf.h"

#if defined(__cplusplus) && (__cplusplus >  201703L)
#define WLB_LOG(text, ...) \
    do {                    \
        printf(text __VA_OPT__(,) __VA_ARGS__);                        \
    } while(0)              \

#else

#define WLB_LOG(text, ...) \
    do {                    \
        printf(text, ##__VA_ARGS__);                        \
    } while(0)              \

#endif