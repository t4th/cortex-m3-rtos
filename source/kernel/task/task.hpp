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

    // todo: consider making this an pointer
    typedef struct Id // This is struct for typesafety
    {
        volatile uint32_t m_id;
        
        inline volatile Id & operator= (volatile Id & a_other) volatile
        {
            m_id = a_other.m_id; 
            return *this; 
        }
        
        inline Id() : m_id{0U} {}
        

        inline Id(Id & a_other)
        {
            m_id = a_other.m_id; 
        }

        inline Id(volatile Id & a_other)
        {
            m_id = a_other.m_id; 
        }
    } Id;

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
        kernel::internal::common::MemoryBuffer<volatile Task, MAX_NUMBER> m_data;
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

    void destroy( Context & a_context, Id a_id);
        
    namespace priority
    {
        kernel::task::Priority get( Context & a_context, Id a_id);
    }

    namespace state
    {
        kernel::task::State get( Context & a_context, Id a_id);

        void set( Context & a_context, Id a_id, kernel::task::State a_state );
    }

    namespace context
    {
        volatile kernel::hardware::task::Context * get( Context & a_context, Id a_id);
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

    namespace wait
    {
        volatile Conditions & getRef( Context & a_context, Id a_id);
    }
}
