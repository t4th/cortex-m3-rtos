#pragma once

#include "hardware/hardware.hpp"
#include "common/memory_buffer.hpp"

#include <kernel.hpp>

namespace kernel::internal::task
{
    constexpr uint32_t priorities_count = 
        static_cast< uint32_t>( kernel::task::Priority::Idle) + 1U;

    // todo: consider it type strong
    typedef uint32_t Id;

    struct Task
    {
        uint32_t                        m_sp;
        hardware::task::Context         m_context;
        hardware::task::Stack           m_stack;
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
        volatile common::MemoryBuffer< Task, max_number> m_data{};
    };

    typedef void( *TaskRoutine)(void);
    

    inline bool create(
        Context &               a_context,
        TaskRoutine             a_task_routine,
        kernel::task::Routine   a_routine,
        kernel::task::Priority  a_priority,
        Id *                    a_id,
        void *                  a_parameter,
        bool                    a_create_suspended
        )
    {
        // Verify arguments.
        if ( nullptr == a_routine)
        {
            return false;
        }
        
        // Create new Task object.
        uint32_t new_item_id;
        
        if ( false == a_context.m_data.allocate( new_item_id))
        {
            return false;
        }
        
        // Initialize new Task object.
        volatile Task & new_task = a_context.m_data.at( new_item_id);
        
        new_task.m_priority = a_priority;
        new_task.m_routine = a_routine;
        new_task.m_stack.init( reinterpret_cast< uint32_t>( a_task_routine));
        new_task.m_sp = new_task.m_stack.getStackPointer();
        new_task.m_parameter = a_parameter;
        
        if ( true == a_create_suspended)
        {
            new_task.m_state = kernel::task::State::Suspended;
        }
        else
        {
            new_task.m_state = kernel::task::State::Ready;
        }
        
        if ( nullptr != a_id)
        {
            // Task ID, is task index in memory buffer by design.
            *a_id = new_item_id;
        }
        
        return true;
    }

    
    inline void destroy( Context & a_context, Id & a_id)
    {
        a_context.m_data.free( a_id);
    }

    namespace priority
    {
        inline kernel::task::Priority get( Context & a_context, volatile Id & a_id)
        {
            return a_context.m_data.at( a_id).m_priority;
        }
    }

    namespace state
    {
        inline kernel::task::State get( Context & a_context, Id & a_id)
        {
            return a_context.m_data.at( a_id).m_state;
        }

        inline void set( Context & a_context, volatile Id & a_id, kernel::task::State a_state )
        {
            a_context.m_data.at( a_id).m_state = a_state;
        }
    }

    namespace context
    {
        inline volatile hardware::task::Context * get( Context & a_context, Id & a_id)
        {
            return &a_context.m_data.at( a_id).m_context;
        }
    }
    
    namespace sp
    {
        inline uint32_t get( Context & a_context, Id & a_id)
        {
            return a_context.m_data.at( a_id).m_sp;
        }

        inline void set( Context & a_context, Id & a_id, uint32_t a_new_sp )
        {
            a_context.m_data.at( a_id).m_sp = a_new_sp;
        }
    }

    namespace routine
    {
        inline kernel::task::Routine get( Context & a_context, Id & a_id)
        {
            return a_context.m_data.at( a_id).m_routine;
        }
    }

    namespace parameter
    {
        inline void * get( Context & a_context, Id & a_id)
        {
            return a_context.m_data.at( a_id).m_parameter;
        }
    }

    namespace wait
    {
        namespace result
        {
            inline kernel::sync::WaitResult get( Context & a_context, Id & a_id)
            {
                return a_context.m_data.at( a_id).m_result;
            }

            inline void set( Context & a_context, Id & a_id, kernel::sync::WaitResult a_value)
            {
                a_context.m_data.at( a_id).m_result = a_value;
            }
        }

        namespace last_signal_index
        {
            inline uint32_t get( Context & a_context, Id & a_id)
            {
                return a_context.m_data.at( a_id).m_last_signal_index;
            }

            inline void set( Context & a_context, Id & a_id, uint32_t a_value)
            {
                a_context.m_data.at( a_id).m_last_signal_index = a_value;
            }
        }
    }
}
