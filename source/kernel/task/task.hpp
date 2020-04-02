#pragma once

#include <cstdint>

namespace kernel::task
{
    typedef void(*Routine)(void);  // TODO: Add argument.
    
    enum class Priority
    {
        High,
        Medium,
        Low
    };
    
    enum class State
    {
        Suspended,
        Waiting,
        Ready,
        Running
    };
    
    bool create(
        Routine     a_routube,
        Priority    a_priority = kernel::task::Priority::Low,
        uint32_t *  a_handle = nullptr,
        bool        a_create_suspended = false
        );
}
