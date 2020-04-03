#pragma once

#include <cstdint>

namespace kernel::hardware
{
    constexpr uint32_t task_stack_size = 32;
    
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
                uint32_t    m_data[task_stack_size];
        };
    }
    
    void init();
    void start();
}
