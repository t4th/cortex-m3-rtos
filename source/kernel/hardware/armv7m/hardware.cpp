#include <hardware.hpp>
#include <kernel.hpp>

#include <stm32f10x.h>

namespace kernel::hardware
{
    constexpr uint32_t core_clock_freq_hz{72'000'000};
    constexpr uint32_t target_systick_timestamp_us{1000};
    constexpr uint32_t systick_prescaler{core_clock_freq_hz/target_systick_timestamp_us};
    
    void init()
    {
        SysTick_Config(systick_prescaler - 1);
        
        ITM->TCR |= ITM_TCR_ITMENA_Msk;  // ITM enable
        ITM->TER = 1UL;                  // ITM Port #0 enable
    }
    
    void start()
    {
        // setup interrupts
        NVIC_SetPriority(SysTick_IRQn, 10); // systick should be higher than pendsv
        NVIC_SetPriority(PendSV_IRQn, 12);
        
        NVIC_EnableIRQ(SysTick_IRQn);
        NVIC_EnableIRQ(PendSV_IRQn);
    }
}

// TODO: Make this somehow private to kernel::hardware or nameless namespace.
// This is workaround to use C++ variables symbols in assembly language.
volatile uint32_t * current_task_context = 0;
volatile uint32_t * next_task_context = 0;
volatile uint32_t skip_store = 0;

extern "C"
{
    void SysTick_Handler(void)
    {
        __disable_irq();
        
        kernel::tick();
        
        __DSB(); // This is not needed in this case, due to write buffer being cleared on interrupt exit,
                 // but it is nice to have explicit information that memory write delay is taken into account.
        __enable_irq();
    }
}

extern "C"
{
    void PendSV_Handler(void)
    {
        __ASM
        (
        "CPSID I;"

        "store_current_task: "
        "ldr r0, =current_task_context; "
        "ldr r1, [r0];"
        "stm r1, {r4-r11};"
        
        "load_next_task:"
        "ldr r0, =next_task_context;"
        "ldr r1, [r0];"
        "ldm r1, {r4-r11};"
        
        // Set skip_store to 0.
        "mov r0, #0;"
        "nop;"
        "str r0, [r2];"
        
        // 0xFFFFFFFD in r0 means 'return to thread mode'.
        // Without this PendSV would return to SysTick
        // losing current thread state along the way.
        "ldr r0, =0xFFFFFFFD ;"
        "CPSIE I; "
        "bx r0"
        );
    }
}
