#include "hardware/hardware.hpp"

#include <stm32f10x.h>

#include <cassert>

// This file contain hardware specific implementation.
// In current case its stm32f103ze clocked at 72MHz.

// TODO: Make this somehow private to kernel::hardware or nameless namespace.
// This is workaround to use C++ variables symbols in inline assembly.
volatile kernel::internal::hardware::task::Context * current_task_context;
volatile kernel::internal::hardware::task::Context * next_task_context;

namespace
{
    // Variables used to calculate SysTick prescaler to reach 1 ms timestamp.
    // This is derived from formula:
    // TARGET_SYSTICK_TIMESTAMP_HZ = CORE_CLOCK_FREQ_HZ / SYSTICK_PRESCALER,
    // where SYSTICK_PRESCALER is searched value.
    constexpr uint32_t target_systick_timestamp_hz{ 1000U};
    constexpr uint32_t systick_prescaler{ kernel::internal::hardware::core_clock_freq_hz / target_systick_timestamp_hz};

    // Maxium number of vendor defined interrupts for ARMv7-m.
    constexpr uint32_t maximum_priority_number{ 240U};

    // Interrupt priority configuration for ARMv7-m.
    constexpr uint32_t number_of_preemption_priority_bits{ 2U};
    constexpr uint32_t number_of_sub_priority_bits{ 2U};
    constexpr uint32_t number_of_total_priority_bits{ __NVIC_PRIO_BITS};
}

// User-level hardware interface.
namespace kernel::hardware
{
    namespace interrupt
    {
        // Hardware priority config.
        namespace priority
        {
            // This is replacement of CMSIS __NVIC_SetPriority, which is using fixed amount of priority bits.
            constexpr void set(
                IRQn_Type   a_interrupt_number,
                Preemption  a_preemption_priority,
                Sub         a_sub_priority
            )
            {
                // Pre-emption priority value 0 is reserved. Set enum underlying value to always skip it.
                uint32_t preemption_priority = static_cast< uint32_t> ( a_preemption_priority) + 1U;
                uint32_t sub_priority = static_cast< uint32_t> ( a_sub_priority);

                // Note: For a processor configured with less than eight bits of priority,
                //       the lower bits of the register are always 0.

                //       In this case, STM32 uses 4 bits, so result in 8 bit register would be
                //       xx.yy0000, where xx is pre-emption priority, and yy is sub-priority.
                uint8_t new_value = static_cast< uint8_t>(
                    ( preemption_priority << ( 8U - number_of_preemption_priority_bits)) |
                    ( sub_priority << ( 8U - number_of_total_priority_bits))
                    ) & 0xFFUL;

                // Note: signed integer is used, because core interrupts use negative priorities.
                int32_t priority_number = static_cast< int32_t>( a_interrupt_number);

                if ( priority_number >= 0)
                {
                    // Configure vendor interrupts.
                    NVIC->IP[ priority_number] = new_value;
                }
                else
                {
                    // Configure core interrupts.
                    uint32_t system_priority_number = ( priority_number & 0xFUL) - 4UL;
                    SCB->SHP[ system_priority_number] = new_value;
                }
            }

            void set(
                uint32_t    a_vendor_interrupt_id,
                Preemption  a_preemption_priority,
                Sub         a_sub_priority
            )
            {
                if ( a_vendor_interrupt_id < maximum_priority_number)
                {
                    IRQn_Type interrupt_number = static_cast< IRQn_Type> ( a_vendor_interrupt_id);

                    set( interrupt_number, a_preemption_priority, a_sub_priority);
                }
                else
                {
                    assert ( true);
                }
            }
        }

        void enable( uint32_t a_vendor_interrupt_id)
        {
            if ( a_vendor_interrupt_id < maximum_priority_number)
            {
                IRQn_Type interrupt_number = static_cast< IRQn_Type> ( a_vendor_interrupt_id);

                NVIC_EnableIRQ( interrupt_number);
            }
            else
            {
                assert ( true);
            }
        }

        void wait()
        {
            __WFI();
        }
    }

    namespace critical_section
    {
        // TODO: test if compiler is not re-ordering this functions.
        void enter( Context & a_context, interrupt::priority::Preemption a_preemption_priority)
        {
            // Pre-emption priority value 0 is reserved. Set enum underlying value to always skip it.
            uint32_t preemption_priority = static_cast< uint32_t> ( a_preemption_priority) + 1U;
            uint32_t new_value = preemption_priority << ( 8U - number_of_preemption_priority_bits);

            // Setting BASEPRI to 0 has no effect, so it is most likely bug.
            assert( 0U != new_value);

            // Store previous priority;
            a_context.m_local_data = __get_BASEPRI();

            // Use BASEPRI register value as critical section for provided pre-emption priority.
            __set_BASEPRI( new_value);

            __DSB(); // Used as compiler re-order barrer and forces that m_local_data is not cached.
        }

        void leave( Context & a_context)
        {
            __DSB(); // Used as compiler re-order barrer
            __set_BASEPRI( a_context.m_local_data);
        }
    }

    namespace debug
    {
        void init()
        {
            ITM->TCR |= ITM_TCR_ITMENA_Msk;  // ITM enable
            ITM->TER = 1UL;                  // ITM Port #0 enable
        }

        void putChar( char c)
        {
            ITM_SendChar( c);
        }

        void print( const char * s)
        {
            while ( '\0' != *s)
            {
                debug::putChar( *s);
                ++s;
            }
        }

        void setBreakpoint()
        {
            __BKPT( 0);
        }
    }
}

// Kernel-level hardware interface.
namespace kernel::internal::hardware
{
    namespace interrupt
    {
        void init()
        {
            // This value indicate shift number needed to set binary point position in
            // Interrupt priority level field, PRI_N[7:0].

            // Since number of bits used for pre-emption priority is 4, binary point should
            // result as follows: xx.yyyyyy, where xx are number of bits used by pre-emption priority
            // and yyyyyy are bits used by sub-priority.
            constexpr uint32_t binary_point_shift =
                7U - number_of_preemption_priority_bits; 

            NVIC_SetPriorityGrouping( binary_point_shift);
        }

        void start()
        {
            // Note: Enable SysTick last, since it is the only PendSV trigger.
            NVIC_EnableIRQ( SVCall_IRQn);
            NVIC_EnableIRQ( PendSV_IRQn);
            NVIC_EnableIRQ( SysTick_IRQn);
        }
    }

    void syscall( SyscallId a_id)
    {
        __ASM(" DMB"); // Complete all explicit memory transfers
        
        switch( a_id)
        {
        case SyscallId::LoadNextTask:
            __ASM(" SVC #0");
            break;
        case SyscallId::ExecuteContextSwitch:
            __ASM(" SVC #1");
            break;
        }
    }

    void init()
    {
        SysTick_Config( systick_prescaler - 1U);

        // TODO: enable only in debug mode.
        kernel::hardware::debug::init();

        interrupt::init();

        // Note: SysTick and PendSV interrupts use the same priority,
        //       to remove the need of critical section for use of shared
        //       kernel data. It is actually recommended in ARM reference manual.
        using namespace kernel::hardware::interrupt;

        priority::set( SVCall_IRQn, priority::Preemption::Kernel, priority::Sub::Low);
        priority::set( PendSV_IRQn, priority::Preemption::Kernel, priority::Sub::Low);
        priority::set( SysTick_IRQn, priority::Preemption::Kernel, priority::Sub::Low);
    }
    
    void start()
    {
        interrupt::start();
    }

    namespace sp
    {
        uint32_t get()
        {
            // TODO: Thread mode should be used in final version
            return __get_PSP();
        }

        void set( uint32_t a_new_sp)
        {
            // TODO: Thread mode should be used in final version
            __set_PSP( a_new_sp);
        }
    }

    namespace context::current
    {
        void set( volatile task::Context * a_context)
        {
            current_task_context = a_context;
        }
    }

    namespace context::next
    {
        void set( volatile task::Context * a_context)
        {
            next_task_context = a_context;
        }
    }
}

namespace kernel::internal::hardware::task
{
    // This function initialize default stack frame for each task.
    void Stack::init( uint32_t a_routine_address) volatile
    {
        // This is magic number and does not hold any meaning. It help tracking stack overflows.
        constexpr uint32_t default_general_purpose_register_value = 0xCDCD'CDCDU;

        // This is default value set by CPU after reset.
        // If interrupt return with this value set, it will cause fault error.
        constexpr uint32_t default_link_register_value = 0xFFFF'FFFFU;

        // Set Instruction set to Thumb.
        constexpr uint32_t program_status_register_value = 0x01000'000U;

        // CPU uses full descending stack, thats why starting stack frame is placed at the bodom.
        m_data[ stack_size - 8U] = default_general_purpose_register_value; // R0
        m_data[ stack_size - 7U] = default_general_purpose_register_value; // R1
        m_data[ stack_size - 6U] = default_general_purpose_register_value; // R2
        m_data[ stack_size - 5U] = default_general_purpose_register_value; // R3
        m_data[ stack_size - 4U] = default_general_purpose_register_value; // R12
        m_data[ stack_size - 3U] = default_link_register_value;            // R14 - Link register
        m_data[ stack_size - 2U] = a_routine_address;                      // R15 - Program counter
        m_data[ stack_size - 1U] = program_status_register_value;          // xPSR - Program Status Register
    }
    
    uint32_t Stack::getStackPointer() volatile
    {
        return reinterpret_cast< uint32_t>( &m_data[ stack_size - 8U]);
    }
}

extern "C"
{
    // TOOD: print nice error log in case of failed run-time assert.
    void __aeabi_assert(
        const char * expr,
        const char * file,
        int line
    )
    {
        kernel::hardware::debug::setBreakpoint();
        for (;;);
    }
    
    inline __attribute__ (( naked )) void LoadTask(void)
    {
        __ASM(" CPSID I\n");

        // Load task context from address provided by next_task_context.
        __ASM(" ldr r0, =next_task_context\n");
        __ASM(" ldr r1, [r0]\n");
        __ASM(" ldm r1, {r4-r11}\n");

        // 0xFFFFFFFD in r0 means 'return to thread mode' (use PSP).
        // TODO: first task should be initialized to Thread Mode
        __ASM(" ldr r0, =0xFFFFFFFD \n");
        __ASM(" CPSIE I \n");
        
        // This is not needed in this case, due to write buffer being cleared on interrupt exit,
        // but it is nice to have explicit information that memory write delay is taken into account.
        __ASM(" DSB"); // Complete all explicit memory transfers
        __ASM(" ISB"); // flush instruction pipeline
        
        __ASM(" bx r0");
    }

    void SVC_Handler_Main( unsigned int * svc_args)
    {
        // This handler implementation is taken from official reference manual.
        // svc_args points to context stored when SVC interrupt was activated.
 
        // Stack contains:
        // r0, r1, r2, r3, r12, r14, the return address and xPSR
        // First argument (r0) is svc_args[0]
        unsigned int  svc_number = ( ( char * )svc_args[ 6U ] )[ -2U ] ; // TODO: simplify this bullshit obfuscation
        switch( svc_number )
        {
            case 0U:       // SyscallId::LoadNextTask:
            {
                kernel::internal::loadNextTask();
                __DSB(); // Complete unfinished memory transfers from loadNextTask.
                LoadTask();
                break;
            }
            case 1U:       // SyscallId::ExecuteContextSwitch
            {
                kernel::internal::switchContext();
                __DSB(); // Complete unfinished memory transfers from switchContext.
                __ISB(); // Flush instructions in pipeline before entering PendSV.
                SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; // Set PendSV_Handler to pending state so it can tail chain from SVC.
                break;
            }
            default:      // unknown SVC
                break;
        }
    }

    __attribute__ (( naked )) void SVC_Handler(void)
    {
        // This handler implementation is taken from official reference manual. Since SVC assembly instruction store argument in opcode itself,
        // code bellow track instruction address that invoked SVC interrupt via context stored when interrupt was activated.
        __ASM(" TST lr, #4\n");  // lr AND #4 - Test if masked bits are set. 
        __ASM(" ITE EQ\n"        // Next 2 instructions are conditional. EQ - equal - Z(zero) flag == 1. Check if result is 0.
        " MRSEQ r0, MSP\n"       // Move the contents of a special register to a general-purpose register. It block with EQ.
        " MRSNE r0, PSP\n");     // It block with NE -> z == 0 Not equal
        __ASM(" B SVC_Handler_Main\n");
    }

    void SysTick_Handler(void)
    {
        // TODO: Make this function explicitly inline.
        bool execute_context_switch = kernel::internal::tick();

        __DSB(); // Complete unfinished memory transfers from tick function.

        if ( execute_context_switch)
        {
            SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; // Set PendSV interrupt to pending state.
        }
    }
    
    __attribute__ (( naked )) void PendSV_Handler(void) // Use 'naked' attribute to remove C ABI, because return from interrupt must be set manually.
    {
        __ASM(" CPSID I\n");
        
        // Store current task at address provided by current_task_context.
        __ASM(" ldr r0, =current_task_context\n");
        __ASM(" ldr r1, [r0]\n");
        __ASM(" stm r1, {r4-r11}\n");
        
        // Load task context from address provided by next_task_context.
        __ASM(" ldr r0, =next_task_context\n");
        __ASM(" ldr r1, [r0]\n");
        __ASM(" ldm r1, {r4-r11}\n");
        
        // 0xFFFFFFFD in r0 means 'return to thread mode' (use PSP).
        // Without this PendSV would return to SysTick
        // losing current thread state along the way.
        __ASM(" ldr r0, =0xFFFFFFFD \n");
        __ASM(" CPSIE I \n");
        
        // This is not needed in this case, due to write buffer being cleared on interrupt exit,
        // but it is nice to have explicit information that memory write delay is taken into account.
        __ASM(" DSB"); // Complete all explicit memory transfers
        __ASM(" ISB"); // Flush instruction pipeline before branching to next task.
        
        __ASM(" bx r0");
    }
}
