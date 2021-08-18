#pragma once

#include "config/config.hpp"
#include "common/memory_buffer.hpp"

#include "../kernel.hpp"

namespace kernel::internal::timer
{
    // Type strong index of Timer.
    enum class Id : uint32_t{};

    enum class State
    {
        Stopped, // Timer created.
        Started, // Timer started.
        Finished // Timer reached required interval.
    };

    struct Timer
    {
        TimeMs  m_start;
        TimeMs  m_interval;
        TimeMs  m_current;
        State   m_state;
    };
    
    // Type strong memory index for allocated Timer type.
    typedef common::MemoryBuffer< Timer, max_number>::Id MemoryBufferIndex;

    struct Context
    {
        volatile common::MemoryBuffer< Timer, max_number> m_data{};
    };

    inline bool create(
        Context &   a_context,
        Id &        a_id,
        TimeMs &    a_start,
        TimeMs &    a_interval
    )
    {
        kernel::hardware::CriticalSection critical_section{ critical_section_priority};

        // Create new Timer object.
        MemoryBufferIndex new_item_id;

        if ( false == a_context.m_data.allocate( new_item_id))
        {
            return false;
        }

        a_id = static_cast< Id>( new_item_id);

        // Initialize new Timer object.
        volatile Timer & new_timer = a_context.m_data.at( new_item_id);

        new_timer.m_start = a_start;
        new_timer.m_interval = a_interval;
        new_timer.m_current = a_start;

        new_timer.m_state = State::Stopped;

        return true;
    }

    inline void destroy( Context & a_context, Id & a_id)
    {
        kernel::hardware::CriticalSection critical_section{ critical_section_priority};

        a_context.m_data.free( static_cast< MemoryBufferIndex> ( a_id));
    }

    inline void start( Context & a_context, Id & a_id)
    {
        kernel::hardware::CriticalSection critical_section{ critical_section_priority};

        a_context.m_data.at( static_cast< MemoryBufferIndex> ( a_id)).m_state = State::Started;
    }

    inline void restart( Context & a_context, Id & a_id)
    {
        kernel::hardware::CriticalSection critical_section{ critical_section_priority};

        volatile Timer & timer = a_context.m_data.at( static_cast< MemoryBufferIndex> ( a_id));

        timer.m_start = timer.m_current;
        timer.m_state = State::Started;
    }

    inline void stop( Context & a_context, Id & a_id)
    {
        kernel::hardware::CriticalSection critical_section{ critical_section_priority};

        a_context.m_data.at( static_cast< MemoryBufferIndex> ( a_id)).m_state = State::Stopped;
    }

    inline State getState( Context & a_context, Id & a_id)
    {
        kernel::hardware::CriticalSection critical_section{ critical_section_priority};

        return a_context.m_data.at( static_cast< MemoryBufferIndex> ( a_id)).m_state;
    }

    // Note: No critical section here, since this function is called from within kernel::internal::tick.
    inline void tick( Context & a_context, TimeMs & a_current)
    {
        for ( uint32_t i = 0U; i < max_number; ++i)
        {
            if ( true == a_context.m_data.isAllocated( static_cast< MemoryBufferIndex> ( i)))
            {
                volatile Timer & current_timer = a_context.m_data.at( static_cast< MemoryBufferIndex> ( i));
                current_timer.m_current = a_current;

                if ( State::Started == current_timer.m_state)
                {
                    if ( ( current_timer.m_current - current_timer.m_start) > current_timer.m_interval)
                    {
                        current_timer.m_state = State::Finished;
                    }
                }
            }
        }
    }
}
