#pragma once

#include <cstdint>
#include <hardware.hpp>
#include <kernel.hpp>

namespace kernel::internal::task
{
    constexpr uint32_t MAX_TASK_NUMBER = 16U;

    constexpr uint32_t PRIORITIES_COUNT = kernel::task::Priority::Idle + 1U;
    
    bool create(
        kernel::task::Routine   a_routine,
        kernel::task::Priority  a_priority = kernel::task::Priority::Low,
        kernel::task::Id *      a_handle = nullptr,
        bool                    a_create_suspended = false
        );

    void destroy(kernel::task::Id a_id);
        
    namespace priority
    {
        kernel::task::Priority get(kernel::task::Id a_id);
    }

    namespace context
    {
        kernel::hardware::task::Context *  get(kernel::task::Id a_id);
    }
    
    namespace sp
    {
        uint32_t get(kernel::task::Id a_id);
        void set(kernel::task::Id a_id, uint32_t a_new_sp);
    }

    namespace routine
    {
        kernel::task::Routine get(kernel::task::Id a_id);
    }
}
