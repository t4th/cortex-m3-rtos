#pragma once

#include <cstdint>

namespace kernel::hardware
{
    constexpr uint32_t TASK_STACK_SIZE = 32U;
    
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
                uint32_t    m_data[TASK_STACK_SIZE];
        };
    }
    
    enum class SyscallId : uint8_t
    {
        StartFirstTask,
        ExecuteContextSwitch
    };

    void syscall(SyscallId a_id);

    void init();
    void start();

    namespace sp
    {
        uint32_t get();
        void set(uint32_t);
    }
}
