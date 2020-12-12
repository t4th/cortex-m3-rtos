#pragma once

#include <hardware.hpp>
#include <kernel.hpp>
#include <memory_buffer.hpp>

namespace kernel::internal::task
{
    constexpr uint32_t MAX_NUMBER = 16U;
    constexpr uint32_t PRIORITIES_COUNT = static_cast<uint32_t>(kernel::task::Priority::Idle) + 1U;

    // todo: consider it type strong
    typedef uint32_t Id;

    struct Task
    {
        uint32_t                        m_sp;
        kernel::hardware::task::Context m_context;
        kernel::hardware::task::Stack   m_stack;
        kernel::task::Priority          m_priority;
        kernel::task::State             m_state;
        void *                          m_parameter;
        kernel::task::Routine           m_routine;

        // Results from Wait for Object function.
        kernel::sync::WaitResult        m_result;
        uint32_t                        m_last_signal_index;
    };

    struct Context
    {
        volatile kernel::internal::common::MemoryBuffer<Task, MAX_NUMBER> m_data{};
    };

    typedef void(*TaskRoutine)(void);
    
    bool create(
        Context &               a_context,
        TaskRoutine             a_task_routine,
        kernel::task::Routine   a_routine,
        kernel::task::Priority  a_priority = kernel::task::Priority::Low,
        Id *                    a_id = nullptr,
        void *                  a_parameter = nullptr,
        bool                    a_create_suspended = false
        );

    
    inline void destroy( Context & a_context, Id & a_id)
    {
        a_context.m_data.free(a_id);
    }

    namespace priority
    {
        inline kernel::task::Priority get( Context & a_context, volatile Id & a_id)
        {
            return a_context.m_data.at(a_id).m_priority;
        }
    }

    namespace state
    {
        inline kernel::task::State get( Context & a_context, Id & a_id)
        {
            return a_context.m_data.at(a_id).m_state;
        }

        inline void set( Context & a_context, volatile Id & a_id, kernel::task::State a_state )
        {
            a_context.m_data.at(a_id).m_state = a_state;
        }
    }

    namespace context
    {
        inline volatile kernel::hardware::task::Context * get( Context & a_context, Id & a_id)
        {
            return &a_context.m_data.at(a_id).m_context;
        }
    }
    
    namespace sp
    {
        inline uint32_t get( Context & a_context, Id & a_id)
        {
            return a_context.m_data.at(a_id).m_sp;
        }

        inline void set( Context & a_context, Id & a_id, uint32_t a_new_sp )
        {
            a_context.m_data.at(a_id).m_sp = a_new_sp;
        }
    }

    namespace routine
    {
        inline kernel::task::Routine get( Context & a_context, Id & a_id)
        {
            return a_context.m_data.at(a_id).m_routine;
        }
    }

    namespace parameter
    {
        inline void * get( Context & a_context, Id & a_id)
        {
            return a_context.m_data.at(a_id).m_parameter;
        }
    }

    namespace wait
    {
        namespace result
        {
            inline kernel::sync::WaitResult get( Context & a_context, Id & a_id)
            {
                return a_context.m_data.at(a_id).m_result;
            }

            inline void set( Context & a_context, Id & a_id, kernel::sync::WaitResult a_value)
            {
                a_context.m_data.at(a_id).m_result = a_value;
            }
        }

        namespace last_signal_index
        {
            inline uint32_t get( Context & a_context, Id & a_id)
            {
                return a_context.m_data.at(a_id).m_last_signal_index;
            }

            inline void set( Context & a_context, Id & a_id, uint32_t a_value)
            {
                a_context.m_data.at(a_id).m_last_signal_index = a_value;
            }
        }
    }
}
