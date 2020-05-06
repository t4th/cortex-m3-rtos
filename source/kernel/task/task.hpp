#pragma once

#include <cstdint>
#include <hardware.hpp>

namespace kernel::task
{
    typedef void(*Routine)(void);  // TODO: Add argument.
    
    constexpr uint32_t MAX_TASK_NUMBER = 16U;
    constexpr uint32_t PRIORITIES_COUNT = 3U; // TODO: Can this be calculated during compile time?
    
    typedef uint32_t Id;
    
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
        Id *        a_handle = nullptr,
        bool        a_create_suspended = false
        );
        
    namespace priority
    {
        Priority get(Id a_id);
    }

    namespace context
    {
        kernel::hardware::task::Context *  getRef(Id a_id);
    }
    
    namespace sp
    {
        uint32_t get(Id a_id);
        void set(Id a_id, uint32_t a_new_sp);
    }
}
