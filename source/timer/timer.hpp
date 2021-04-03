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
        Time_ms     m_start;
        Time_ms     m_interval;
        State       m_state;
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
        Time_ms &   a_start,
        Time_ms &   a_interval
    )
    {
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

        new_timer.m_state = State::Stopped;

        return true;
    }

    inline void destroy( Context & a_context, Id & a_id)
    {
        a_context.m_data.free( static_cast< MemoryBufferIndex> ( a_id));
    }

    inline void start( Context & a_context, Id & a_id)
    {
        a_context.m_data.at( static_cast< MemoryBufferIndex> ( a_id)).m_state = State::Started;
    }

    inline void stop( Context & a_context, Id & a_id)
    {
        a_context.m_data.at( static_cast< MemoryBufferIndex> ( a_id)).m_state = State::Stopped;
    }

    inline State getState( Context & a_context, Id & a_id)
    {
        return a_context.m_data.at( static_cast< MemoryBufferIndex> ( a_id)).m_state;
    }

    inline void tick( Context & a_context, Time_ms & a_current)
    {
        // TODO: consider memory barrier since this funtion is
        //       called from interrupt handler.

        // TODO: Iterate through all timers, even not allocated.
        //       Limiting branches should be more effective that
        //       late decision trees (and more cache friendly if
        //       applicable).
        for ( uint32_t i = 0U; i < max_number; ++i)
        {
            // Note: For now this check stays due to memory buffer
            //       assert isAllocated when 'at' dereference is used.
            if ( true == a_context.m_data.isAllocated( static_cast< MemoryBufferIndex> ( i)))
            {
                volatile Timer & timer = a_context.m_data.at( static_cast< MemoryBufferIndex> ( i));

                if ( ( a_current - timer.m_start) > timer.m_interval)
                {
                    // TODO: It would be more effective to make 2
                    //       buffers for each timer state.
                    timer.m_state = State::Finished;
                }
            }
        }
    }
}
