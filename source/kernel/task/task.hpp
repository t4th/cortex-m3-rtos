#pragma once

#include <cstdint>

namespace kernel::task
{
    typedef void(*Routine)(void);  // TODO: Add argument.
    
    constexpr uint32_t max_task_number = 16;
    constexpr uint32_t priorities_count = 3; // TODO: Can this be calculated during compile time?
    
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
        Routine     a_routine,
        Priority    a_priority = Priority::Low,
        uint32_t *  a_handle = nullptr,
        bool        a_create_suspended = false
        );
}
