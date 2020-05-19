#pragma once

#include <cstdint>

// User API
namespace kernel
{    
    void init();
    void start();
}

namespace kernel::task
{
    typedef uint32_t Id;
    typedef void(*Routine)(void);  // TODO: Add argument.

    enum Priority
    {
        High = 0U,
        Medium,
        Low,
        Idle
    };

    enum State
    {
        Suspended,
        Waiting,
        Ready,
        Running
    };

    bool create(
        kernel::task::Routine   a_routine,
        kernel::task::Priority  a_priority = kernel::task::Priority::Low,
        kernel::task::Id *      a_handle = nullptr,
        bool                    a_create_suspended = false
    );
}

namespace kernel::internal
{
    bool tick() __attribute__((always_inline));
}
