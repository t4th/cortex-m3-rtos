#pragma once

#include <cstdint>
#include <hardware.hpp> //TODO: create kerneli::api fascade

namespace kernel
{    
    void init();
    void start();
}

namespace kernel
{
    bool tick
        (
        volatile kernel::hardware::task::Context * & a_current_task_context,
        volatile kernel::hardware::task::Context * & a_next_task_context
        ) __attribute__((always_inline));
}
