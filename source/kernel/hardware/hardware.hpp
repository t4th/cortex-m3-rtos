#pragma once

#include <cstdint>

namespace kernel::hardware
{
    constexpr uint32_t TASK_STACK_SIZE = 64U;
    
    namespace task
    {
        struct Context
        {
            uint32_t    r4, r5, r6, r7,
                        r8, r9, r10, r11;
        };
        
        class Stack
        {
            public:
                void        init(uint32_t a_routine);
                uint32_t    getStackPointer();
            private:
                // TODO: Add sanity magic numbers.
                uint32_t    m_data[TASK_STACK_SIZE];
        };
    }
    
    enum class SyscallId : uint8_t
    {
        LoadNextTask,
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
        void set(kernel::hardware::task::Context * a_context);
    }

    namespace context::next
    {
        void set(kernel::hardware::task::Context * a_context);
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
    }
}
