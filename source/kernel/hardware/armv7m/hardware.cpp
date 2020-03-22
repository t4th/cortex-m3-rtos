#include <hardware.hpp>

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

extern "C"
{
    void SysTick_Handler(void)
    {
        int a;
        a++;
    }
}

extern "C"
{
    void PendSV_Handler(void)
    {
    __ASM ("CPSID I");
        
    __ASM ("CPSIE I");
    }
}
