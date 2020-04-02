#pragma once

#include <cstdint>

namespace kernel
{
    void init();
    void start();
    
    struct task_context
    {
        uint32_t *  context; // Context 'must' stay at offset #0!
        uint32_t    sp;
    };
    
    bool tick(volatile task_context & current, volatile task_context & next) __attribute__((always_inline));
}
