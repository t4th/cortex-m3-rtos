cmake_minimum_required(VERSION 3.15.3)

project(waitForMultipleObjects_example C CXX ASM)

set(EXECUTABLE ${PROJECT_NAME}.elf)

set(CMAKE_CXX_STANDARD 17)

set(KERNEL_DIR "../..")
set(LINKER_FILE "${CMAKE_SOURCE_DIR}/${KERNEL_DIR}/external/st/STM32F10x/gcc/STM32F103ZETX_FLASH.ld")
set(COMPILE_FLAGS
        -mcpu=cortex-m3
        -fno-rtti
        -fno-exceptions
        -ffunction-sections -fdata-sections
        -gdwarf-4 -gstrict-dwarf # dwarf standard compatible with keil IDE
        --specs=nano.specs
        --specs=nosys.specs
        -ffreestanding
)
set(C_DEFINITIONS
        -DSTM32F103xE
)

set(MY_SOURCE_FILES
        # main
        main.cpp

        # gcc specific
        ${KERNEL_DIR}/external/st/STM32F10x/gcc/syscalls.c
        ${KERNEL_DIR}/external/st/STM32F10x/gcc/sysmem.c
        ${KERNEL_DIR}/external/st/STM32F10x/gcc/startup_stm32f103zetx.s

        # vendor specific
        ${KERNEL_DIR}/external/st/STM32F10x/system_stm32f1xx.h
        ${KERNEL_DIR}/external/st/STM32F10x/system_stm32f1xx.c
)

add_executable(${EXECUTABLE} ${MY_SOURCE_FILES})

# add kernel module
include_directories(${PROJECT_SOURCE_DIR}/${KERNEL_DIR}/source)
add_compile_options(${COMPILE_FLAGS})
add_definitions(${C_DEFINITIONS})
add_subdirectory(${KERNEL_DIR} rtos)

target_link_libraries(${EXECUTABLE} cortex-m3-rtos)

target_compile_definitions(${EXECUTABLE} PRIVATE ${C_DEFINITIONS})

target_include_directories(${EXECUTABLE} PRIVATE
        ${KERNEL_DIR}/external/arm
        ${KERNEL_DIR}/external/st/STM32F10x
        ${KERNEL_DIR}/external/st/STM32F10x/gcc
        )

target_compile_options(${EXECUTABLE} PRIVATE ${COMPILE_FLAGS})

target_link_options(${EXECUTABLE} PRIVATE
        -mcpu=cortex-m3 -T${LINKER_FILE}
        --specs=nosys.specs -Wl,-Map=${PROJECT_NAME}.map -Wl,--gc-sections -static --specs=nano.specs -mfloat-abi=soft -mthumb -Wl,--start-group -lc -lm -lstdc++ -lsupc++ -Wl,--end-group
        )