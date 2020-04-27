#pragma once

#include <cstdint>
#include <hardware.hpp>

namespace kernel::task
{
    typedef void(*Routine)(void);  // TODO: Add argument.
    
    constexpr uint32_t max_task_number = 16;
    constexpr uint32_t priorities_count = 3; // TODO: Can this be calculated during compile time?
    
    typedef uint32_t id;
    
    enum class Priority : uint32_t
    {
        High,
        Medium,
        Low
    };
    
    enum State
    {
        Suspended,
        Waiting,
        Ready,
        Running
    };
    
    bool create(
        Routine     a_routine,
        Priority    a_priority = Priority::Low,
        id *        a_handle = nullptr,
        bool        a_create_suspended = false
        );
        
    Priority    getPriority(id a_id);
    kernel::hardware::task::Context *  getContext(id a_id);
    uint32_t    getSp(id a_id);
}
