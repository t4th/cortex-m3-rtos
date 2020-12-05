#pragma once

#include <cstdint>

namespace kernel::internal
{
    // Kernel Handler Mode interfaces
    void loadNextTask(); // __attribute__((always_inline));
    void switchContext(); // __attribute__((always_inline));
    bool tick();// __attribute__((always_inline));
}

namespace kernel::hardware
{
    constexpr uint32_t TASK_STACK_SIZE = 192U;
    
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
                void                init(volatile uint32_t a_routine) volatile;
                volatile uint32_t   getStackPointer() volatile;
            private:
                // TODO: Add sanity magic numbers.
                volatile uint32_t    m_data[TASK_STACK_SIZE];
        };
    }
    
    enum class SyscallId : uint8_t
    {
        LoadNextTask = 0U,
        ExecuteContextSwitch
    };

    // When called while interrupts are disabled will cause HardFault exception.
    void syscall(SyscallId a_id);

    void init();
    void start();

    namespace sp
    {
        uint32_t get();
        void set(uint32_t);
    }

    namespace context::current
    {
        void set(volatile kernel::hardware::task::Context * a_context);
    }

    namespace context::next
    {
        void set(volatile kernel::hardware::task::Context * a_context);
    }

    namespace interrupt
    {
        void enableAll();
        void disableAll();
    }

    namespace debug
    {
        void putChar(char c);
        void print(const char * s);
        void setBreakpoint();
    }
}
