#pragma once

#include <cstdint>
#include <hardware.hpp>
#include <kernel.hpp>
#include <memory_buffer.hpp>

namespace kernel::internal::task
{
    constexpr uint32_t MAX_TASK_NUMBER = 16U;
    constexpr uint32_t PRIORITIES_COUNT = kernel::task::Priority::Idle + 1U;

    typedef struct // This is struct for typesafety
    {
        uint32_t m_id;
    } Id;

    struct Task
    {
        uint32_t                        m_sp;
        kernel::hardware::task::Context m_context;
        kernel::hardware::task::Stack   m_stack;
        kernel::task::Priority          m_priority;
        kernel::task::State             m_state;
        void *                          m_parameter;
        kernel::task::Routine           m_routine;
    };

    struct Context
    {
        kernel::internal::common::MemoryBuffer<Task, kernel::internal::task::MAX_TASK_NUMBER> m_data;
    };

    typedef void(*TaskRoutine)(void);
    
    bool create(
        Context &               a_context,
        TaskRoutine             a_task_routine,
        kernel::task::Routine   a_routine,
        kernel::task::Priority  a_priority = kernel::task::Priority::Low,
        Id *                    a_handle = nullptr,
        void *                  a_parameter = nullptr,
        bool                    a_create_suspended = false
        );

    void destroy( Context &   a_context, Id a_id);
        
    namespace priority
    {
        kernel::task::Priority get( Context & a_context, Id a_id);
    }

    namespace context
    {
        kernel::hardware::task::Context *  get( Context & a_context, Id a_id);
    }
    
    namespace sp
    {
        uint32_t get( Context & a_context, Id a_id);

        void set( Context & a_context, Id a_id, uint32_t a_new_sp );
    }

    namespace routine
    {
        kernel::task::Routine get( Context & a_context, Id a_id);
    }

    namespace parameter
    {
        void * get( Context & a_context, Id a_id);
    }
}
