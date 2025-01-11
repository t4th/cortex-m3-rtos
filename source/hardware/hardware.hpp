#pragma once

#include "config/config.hpp"

namespace kernel::internal
{
    // Kernel Handler Mode interfaces.
    void loadNextTask();
    void switchContext();
    bool tick();
}

namespace kernel::internal::hardware
{
    namespace task
    {
        struct Context
        {
            volatile uint32_t   r4, r5, r6, r7,
                                r8, r9, r10, r11;
        };
        
        class Stack
        {
            public:
                void        init( uint32_t a_routine_address) volatile;
                uint32_t    getStackPointer() volatile;

            private:
                volatile uint32_t    m_data[ stack_size];
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
        void set( volatile task::Context * a_context);
    }

    namespace context::next
    {
        void set( volatile task::Context * a_context);
    }

    namespace utility
    {
        void memoryBarrier();
    }
}
