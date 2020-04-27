#pragma once

#include <cstdint>

namespace kernel
{    
    void init();
    void start();
}

namespace kernel::hardware
{
    struct task_context
    {
        void *  context; // Context 'must' stay at offset #0!
        uint32_t    sp;
    };
    
    bool tick(volatile task_context & current, volatile task_context & next) __attribute__((always_inline));
}
