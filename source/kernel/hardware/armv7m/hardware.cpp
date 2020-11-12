#include <hardware.hpp>
#include <kernel.hpp> // TODO: change to kernel_internal

#include <stm32f10x.h>

// TODO: Make this somehow private to kernel::hardware or nameless namespace.
// This is workaround to use C++ variables symbols in inline assembly.
volatile kernel::hardware::task::Context * current_task_context;
volatile kernel::hardware::task::Context * next_task_context;

namespace kernel::hardware
{
    constexpr uint32_t core_clock_freq_hz{72'000'000};
    constexpr uint32_t target_systick_timestamp_us{1000};
    constexpr uint32_t systick_prescaler{core_clock_freq_hz/target_systick_timestamp_us};
    
    namespace debug
    {
        void init()
        {
            ITM->TCR |= ITM_TCR_ITMENA_Msk;  // ITM enable
            ITM->TER = 1UL;                  // ITM Port #0 enable
        }
        void putChar(char c)
        {
            ITM_SendChar(c);
        }

        void print(const char * s)
        {
            while (*s != '\0')
            {
                kernel::hardware::debug::putChar(*s);
                ++s;
            }
        }
    }

    void syscall(SyscallId a_id)
    {
        switch(a_id)
        {
        case SyscallId::StartFirstTask:
            __ASM("SVC #0");
            break;
        case SyscallId::ExecuteContextSwitch:
            __ASM("SVC #1");
            break;
        case SyscallId::LoadNextTask:
            __ASM("SVC #2");
            break;
        }
    }

    void init()
    {
        SysTick_Config(systick_prescaler - 1);
        
        debug::init();

        // Setup interrupts.
        // Set priorities - lower number is higher priority
        NVIC_SetPriority(SVCall_IRQn, 0);
        NVIC_SetPriority(SysTick_IRQn, 1);
        NVIC_SetPriority(PendSV_IRQn, 2);
    }
    
    void start()
    {
        // Enable interrupts
        NVIC_EnableIRQ(SVCall_IRQn);
        NVIC_EnableIRQ(SysTick_IRQn);
        NVIC_EnableIRQ(PendSV_IRQn);
    }

    namespace sp
    {
        uint32_t get()
        {
            // TODO: Thread mode should be used in final version
            return __get_PSP();
        }

        void set(uint32_t a_new_sp)
        {
            // TODO: Thread mode should be used in final version
            __set_PSP(a_new_sp);
        }
    }

    namespace context::current
    {
        void set(kernel::hardware::task::Context * a_context)
        {
            current_task_context = a_context;
        }
    }

    namespace context::next
    {
        void set(kernel::hardware::task::Context * a_context)
        {
            next_task_context = a_context;
        }
    }

    namespace interrupt
    {
        void enableAll()
        {
            __enable_irq();
        }
        void disableAll()
        {
            __disable_irq();
        }
    }
}

namespace kernel::hardware::task
{
    void Stack::init(uint32_t a_routine)
    {
        // TODO: Do something with magic numbers.
        m_data[TASK_STACK_SIZE - 8] = 0xCD'CD'CD'CD; // R0
        m_data[TASK_STACK_SIZE - 7] = 0xCD'CD'CD'CD; // R1
        m_data[TASK_STACK_SIZE - 6] = 0xCD'CD'CD'CD; // R2
        m_data[TASK_STACK_SIZE - 5] = 0xCD'CD'CD'CD; // R3
        m_data[TASK_STACK_SIZE - 4] = 0; // R12
        m_data[TASK_STACK_SIZE - 3] = 0; // LR R14
        m_data[TASK_STACK_SIZE - 2] = a_routine;
        m_data[TASK_STACK_SIZE - 1] = 0x01000000; // xPSR
    }
    
    uint32_t Stack::getStackPointer()
    {
        return (uint32_t)&m_data[TASK_STACK_SIZE - 8];
    }
}

extern "C"
{
    __attribute__ (( naked )) void LoadTask(void)
    {
        __ASM("CPSID I\n");

        // Load task context
        __ASM("ldr r0, =next_task_context\n");
        __ASM("ldr r1, [r0]\n");
        __ASM("ldm r1, {r4-r11}\n");

        // 0xFFFFFFFD in r0 means 'return to thread mode' (use PSP).
        // TODO: first task should be initialized to Thread Mode
        __ASM("ldr r0, =0xFFFFFFFD \n");
        __ASM("CPSIE I \n");
        __ASM("bx r0");
    }

    void SVC_Handler_Main(unsigned int * svc_args)
    {
        // This handler implementation is taken from official reference manual.
        // svc_args points to context stored when SVC interrupt was activated.
 
        // Stack contains:
        // r0, r1, r2, r3, r12, r14, the return address and xPSR
        // First argument (r0) is svc_args[0]
        unsigned int  svc_number = ( ( char * )svc_args[ 6 ] )[ -2 ] ; // TODO: simplify this bullshit obfuscation
        switch( svc_number )
        {
            case 0:       // SyscallId::StartFirstTask
                LoadTask();
                break;
            case 1:       // SyscallId::ExecuteContextSwitch
                SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; // Set PendSV_Handler to pending state.
                break;
            case 2:       // SyscallId::LoadNextTask:
                {
                kernel::internal::loadNextTask();
                LoadTask();
                }
                break;
            default:      // unknown SVC
                break;
        }
    }

    __attribute__ (( naked )) void SVC_Handler(void)
    {
        // This handler implementation is taken from official reference manual. Since SVC assembly instruction store argument in opcode itself,
        // code bellow track instruction address that invoked SVC interrupt via context stored when interrupt was activated.
        __ASM("TST lr, #4\n"); // lr AND #4 - Test if masked bits are set. 
        __ASM("ITE EQ\n" // Next 2 instructions are conditional. EQ - equal - Z(zero) flag == 1. Check if result is 0.
        "MRSEQ r0, MSP\n" // Move the contents of a special register to a general-purpose register. It block with EQ.
        "MRSNE r0, PSP\n"); // It block with NE -> z == 0 Not equal
        __ASM("B SVC_Handler_Main\n");
    }


    void SysTick_Handler(void)
    {
        // TODO: Make this function explicitly inline.
        bool execute_context_switch = kernel::internal::tick();
        
        if (execute_context_switch)
        {
            SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; // Set PendSV interrupt to pending state.
        }
        
        __DMB(); // This is not needed in this case, due to write buffer being cleared on interrupt exit,
                 // but it is nice to have explicit information that memory write delay is taken into account.
    }
    
    __attribute__ (( naked )) void PendSV_Handler(void) // Use 'naked' attribute to remove C ABI, because return from interrupt must be set manually.
    {
        __ASM("CPSID I\n");
        
        // Store current task at address provided by current_task_context.
        __ASM("ldr r0, =current_task_context\n");
        __ASM("ldr r1, [r0]\n");
        __ASM("stm r1, {r4-r11}\n");
        
        // Load task context from address provided by next_task_context.
        __ASM("ldr r0, =next_task_context\n");
        __ASM("ldr r1, [r0]\n");
        __ASM("ldm r1, {r4-r11}\n");
        
        // 0xFFFFFFFD in r0 means 'return to thread mode' (use PSP).
        // Without this PendSV would return to SysTick
        // losing current thread state along the way.
        __ASM("ldr r0, =0xFFFFFFFD \n");
        __ASM("CPSIE I \n");
        __ASM("bx r0");
    }
}
