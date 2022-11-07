cmake_minimum_required(VERSION 3.20)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_C_COMPILER "arm-none-eabi-gcc")
set(CMAKE_CXX_COMPILER "arm-none-eabi-g++")
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_OBJCOPY "arm-none-eabi-objcopy")

file(GLOB_RECURSE STM32_HAL_SRC
        "external/driver/Src/*.c"
        "external/driver/Inc/*.h"
)

file(GLOB_RECURSE SX126X_HAL_SRC
        "main/hal/subghz/*.c"
        "main/hal/subghz/*.h"
)

project(wlboot C CXX ASM)

add_executable(
        wlboot
            "main/stm32wlxx_hal_conf.h" "main/stm32wlxx_it.c" "main/stm32wlxx_it.h"
            "main/startup_stm32wle5xx.s" "main/system_stm32wlxx.c"
            ${STM32_HAL_SRC} ${SX126X_HAL_SRC}
            "main/main.cpp"
            "external/logging/log.c" "external/logging/log.h"
            "main/hal/uart.hpp"
            "main/hal/lpuart.cpp" "main/hal/lpuart.hpp"
            "main/hal/subghz.cpp" "main/hal/subghz.hpp"
            "external/lfbb/lfbb.hpp" "external/lfbb/lfbb_impl.hpp"
        )

target_include_directories(wlboot PUBLIC
        "external/driver/Inc"
        "external/CMSIS/Device/ST/STM32WLxx/Include" "external/CMSIS/Include"
        "main" "external/logging" "external/logging/printf" "external/lfbb"
        "main/hal"
)

if (CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
    message("Build in Release+Log mode")
    target_compile_options(wlboot PUBLIC
            -ffunction-sections -fdata-sections -flto -Os
            -fno-rtti -fno-asynchronous-unwind-tables -fno-omit-frame-pointer -fno-exceptions
            -mthumb -mcpu=cortex-m4 -mfloat-abi=soft
    )

    target_link_options(wlboot PUBLIC
            -T${CMAKE_CURRENT_LIST_DIR}/misc/stm32wle5.ld
            -Wl,-Map=\"${PROJECT_NAME}.map\"
            -lc -lrdimon --specs=rdimon.specs -flto -Os
            -Wl,--gc-sections
            -mthumb -mcpu=cortex-m4 -mfloat-abi=soft
    )

    target_compile_definitions(wlboot PUBLIC
            CORE_CM4=1
            STM32WLE5xx=1
            USE_FULL_LL_DRIVER=1
            USE_HAL_DRIVER=1
    )
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    message("Build in Release with no log mode")
    target_compile_options(wlboot PUBLIC
            -ffunction-sections -fdata-sections -flto -Os
            -fno-rtti -fno-asynchronous-unwind-tables -fno-omit-frame-pointer -fno-exceptions
            -mthumb -mcpu=cortex-m4 -mfloat-abi=soft
    )

    target_link_options(wlboot PUBLIC
            -T${CMAKE_CURRENT_LIST_DIR}/misc/stm32wle5.ld
            -Wl,-Map=\"${PROJECT_NAME}.map\"
            -Wl,--gc-sections -flto -Os -specs=nano.specs -specs=nosys.specs
            -mthumb -mcpu=cortex-m4 -mfloat-abi=soft
    )

    target_compile_definitions(wlboot PUBLIC
            CORE_CM4=1
            STM32WLE5xx=1
            USE_FULL_LL_DRIVER=1
            USE_HAL_DRIVER=1
            DISABLE_LOG=1
    )
else()
    message("Unknown state, debug mode")
    target_compile_options(wlboot PUBLIC
            -Og
            -ffunction-sections -fdata-sections
            -fno-rtti -fno-asynchronous-unwind-tables -fno-omit-frame-pointer -fno-exceptions
            -mthumb -mcpu=cortex-m4 -mfloat-abi=soft
    )

    target_link_options(wlboot PUBLIC
            -Og
            -T${CMAKE_CURRENT_LIST_DIR}/misc/stm32wle5.ld
            -Wl,-Map=\"${PROJECT_NAME}.map\"
            --specs=rdimon.specs -lc -lrdimon
            -Wl,--gc-sections --specs=nano.specs
            -mthumb -mcpu=cortex-m4 -mfloat-abi=soft
    )

    target_compile_definitions(wlboot PUBLIC
            CORE_CM4=1
            STM32WLE5xx=1
            USE_FULL_LL_DRIVER=1
            USE_HAL_DRIVER=1
    )
endif()

set(linker_script "misc/stm32wle5.ld")
set_target_properties(wlboot PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET wlboot POST_BUILD
        COMMAND arm-none-eabi-size --format=berkeley --totals "$<TARGET_FILE:wlboot>"
        COMMENT "Invoking: Cross ARM GNU Print Size")