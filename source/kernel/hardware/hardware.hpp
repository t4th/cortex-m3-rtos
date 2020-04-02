#pragma once

#include <cstdint>

namespace kernel::hardware
{
    struct task_context
    {
        uint32_t    r4, r5, r6, r7,
                    r8, r9, r10, r11;
    };
    
    void init();
    void start();
}
