#pragma once

#include <config.hpp>

namespace kernel::internal
{
    // Kernel Handler Mode interfaces.
    void loadNextTask();
    void switchContext();
    bool tick();
}

namespace kernel::hardware
{
    namespace task
    {
        struct Context
        {
            // TODO: Hide. This is HW specific implementation detail.
            volatile uint32_t   r4, r5, r6, r7,
                                r8, r9, r10, r11;
        };
        
        // TODO: Make it a struct.
        class Stack
        {
            public:
                void        init( uint32_t a_routine) volatile;
                uint32_t    getStackPointer() volatile;

            private:
                volatile uint32_t    m_data[ TASK_STACK_SIZE];
        };
    }
    
    enum class SyscallId : uint8_t
    {
        LoadNextTask = 0U,
        ExecuteContextSwitch
    };

    // When called while interrupts are disabled will cause HardFault exception.
    void syscall( SyscallId a_id);

    void init();
    void start();

    namespace sp
    {
        uint32_t get();
        void set( uint32_t);
    }

    namespace context::current
    {
        void set( volatile kernel::hardware::task::Context * a_context);
    }

    namespace context::next
    {
        void set( volatile kernel::hardware::task::Context * a_context);
    }

    namespace interrupt
    {
        namespace priority
        {
            // Priority groups. Smaller value is higher priority.
            enum class Preemption
            {
                Critical = 1U,
                Kernel = 2U,
                User = 3U
            };

            // Sub-priorities of Priority groups.
            // Smaller value is higher priority within group.
            enum class Sub
            {
                High = 0U,
                Medium = 1U,
                Low = 2U
            };

            // a_vendor_interrupt_id must be set according to vendor data sheet.
            // Vendor interrupts always starts from 0.
            // Using invalid interrupt ID value will result in Undefined Behaviour.
            void set(
                uint32_t    a_vendor_interrupt_id,
                Preemption  a_preemption_priority,
                Sub         a_sub_priority
            );
        }

        // a_vendor_interrupt_id must be set according to vendor data sheet.
        // Vendor interrupts always starts from 0.
        // Using invalid interrupt ID value will result in Undefined Behaviour.
        void enable( uint32_t a_vendor_interrupt_id);

        // Stop core until interrupt occur.
        void wait();
    }

    // Hardware level critical section used for protecting data between interrupts.
    namespace critical_section
    {
        struct Context
        {
            uint32_t m_local_data;
        };

        void enter(
            volatile Context &                  a_context,
            interrupt::priority::Preemption     a_preemption_priority
        );

        void leave( volatile Context & a_context);
    }

    namespace debug
    {
        void putChar( char c);
        void print( const char * s);
        void setBreakpoint();
    }
}
