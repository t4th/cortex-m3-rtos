cmake_minimum_required(VERSION 3.15.3)

project(cortex-m3-rtos C CXX ASM)

set(CMAKE_CXX_STANDARD 17)

include(GNUInstallDirs)

add_library(${PROJECT_NAME} STATIC
        source/kernel.cpp
        source/hardware/armv7m/hardware.cpp
        )

target_compile_definitions(${PROJECT_NAME} PRIVATE
        -DSTM32F103xE
        )

target_include_directories(${PROJECT_NAME} PRIVATE
        source
        external/arm
        external/st/STM32F10x
        external/st/STM32F10x/gcc
        )

target_compile_options(${PROJECT_NAME} PRIVATE
        -mcpu=cortex-m3
        -fno-rtti
        -fno-exceptions
        -ffunction-sections -fdata-sections
        -gdwarf-4 -gstrict-dwarf # dwarf standard compatible with keil IDE
        --specs=nano.specs
        --specs=nosys.specs
        -ffreestanding
        )