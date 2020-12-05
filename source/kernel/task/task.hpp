#pragma once

#include <hardware.hpp>
#include <kernel.hpp>
#include <memory_buffer.hpp>

namespace kernel::internal::task
{
    constexpr uint32_t MAX_NUMBER = 16U;
    constexpr uint32_t PRIORITIES_COUNT = static_cast<uint32_t>(kernel::task::Priority::Idle) + 1U;

    constexpr uint32_t MAX_INPUT_SIGNALS = 16U;
    constexpr uint32_t MAX_OUTPUT_SIGNALS = 16U;

    // todo: consider it type strong
    typedef uint32_t Id;

    namespace wait
    {
        // todo: move it separate header and add functions
        struct Conditions
        {
            enum class Type
            {
                Sleep,
                WaitForObj
            } m_type{};

            // TODO: make these as lists
            internal::common::MemoryBuffer<Handle, MAX_INPUT_SIGNALS>   m_inputSignals{};
            // internal::common::MemoryBuffer<Handle, MAX_OUTPUT_SIGNALS>  m_outputSignals{};
            bool    m_waitForver;
            Time_ms m_interval;
            Time_ms m_start;
            kernel::sync::WaitResult m_result;
        };
    }

    struct Task
    {
        uint32_t                        m_sp;
        kernel::hardware::task::Context m_context;
        kernel::hardware::task::Stack   m_stack;
        kernel::task::Priority          m_priority;
        kernel::task::State             m_state;
        void *                          m_parameter;
        kernel::task::Routine           m_routine;
        wait::Conditions                m_waitConditios;
    };

    struct Context
    {
        volatile kernel::internal::common::MemoryBuffer<Task, MAX_NUMBER> m_data;
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
        inline kernel::task::Priority get( Context & a_context, Id & a_id)
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
        inline volatile Conditions & getRef( Context & a_context, Id & a_id)
        {
            return a_context.m_data.at(a_id).m_waitConditios;
        }
    }
}
