cmake_minimum_required(VERSION 3.20)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/external/stm32-cmake/cmake/stm32_gcc.cmake")
set(STM32_CUBE_WL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/STM32CubeWL")

project(wlboot C CXX ASM)
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

find_package(CMSIS COMPONENTS STM32WL_M4 REQUIRED)
find_package(HAL COMPONENTS STM32WL_M4 REQUIRED)

add_executable(
        wlboot
            "main/main.cpp"
            "external/logging/log.c" "external/logging/log.h"
            "main/hal/uart.hpp"
            "main/hal/lpuart.cpp" "main/hal/lpuart.hpp"
            "main/interrupt/intr_handler.c" "main/interrupt/intr_handler.h"
)

target_include_directories(wlboot PUBLIC "external/logging" "external/logging/printf")
target_link_libraries(wlboot PRIVATE
        HAL::STM32::WL::M4::LL_RCC
        HAL::STM32::WL::M4::LL_GPIO
        HAL::STM32::WL::M4::LL_TIM
        HAL::STM32::WL::M4::LL_PWR
        HAL::STM32::WL::M4::LL_UTILS
        HAL::STM32::WL::M4::LL_EXTI
        HAL::STM32::WL::M4::LL_LPUART
        CMSIS::STM32::WLE5xx::M4
        STM32::NoSys
        STM32::Nano
)

add_subdirectory("external/lfbb")
target_link_libraries(wlboot PRIVATE lfbb)

# See: https://stackoverflow.com/a/72037241
target_compile_options(wlboot PUBLIC
        -ffunction-sections -fdata-sections
        -fno-rtti -fno-asynchronous-unwind-tables -fno-omit-frame-pointer -fno-exceptions
)

target_compile_definitions(wlboot PUBLIC
        USE_FULL_LL_DRIVER=1
)

target_link_options(wlboot PUBLIC -lc -lrdimon --specs=rdimon.specs -Wl,--gc-sections)
stm32_add_linker_script(wlboot PRIVATE misc/WL54CC_M4.ld)

stm32_print_size_of_target(wlboot)