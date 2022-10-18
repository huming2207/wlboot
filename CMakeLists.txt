cmake_minimum_required(VERSION 3.23)
set(CMAKE_C_STANDARD 99)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/external/stm32-cmake/cmake/stm32_gcc.cmake")
set(STM32_CUBE_WL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/STM32CubeWL")

project(wlboot C ASM)
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

find_package(CMSIS COMPONENTS STM32WL_M4 REQUIRED)
find_package(HAL COMPONENTS STM32WL_M4 REQUIRED)

add_executable(wlboot main.c)
target_link_libraries(wlboot PRIVATE
        HAL::STM32::WL::M4::LL_RCC
        HAL::STM32::WL::M4::LL_GPIO
        HAL::STM32::WL::M4::LL_TIM
        HAL::STM32::WL::M4::LL_PWR
        HAL::STM32::WL::M4::LL_UTILS
        CMSIS::STM32::WLE5xx::M4
        STM32::NoSys
        STM32::Nano
)
stm32_print_size_of_target(wlboot)
stm32_