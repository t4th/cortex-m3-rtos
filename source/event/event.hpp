#pragma once

#include "config/config.hpp"
#include "common/memory_buffer.hpp"

#include "../kernel.hpp"

namespace kernel::internal::event
{
    // Type strong index of Event.
    enum class Id : uint32_t{};

    enum class State
    {
        Reset,
        Set
    };

    struct Event
    {
        State           m_state;
        const char *    mp_name;
        bool            m_manual_reset;
    };
    
    // Type strong memory index for allocated Event type.
    typedef common::MemoryBuffer< Event, max_number>::Id MemoryBufferIndex;

    struct Context
    {
        // Note: I considered creating two lists: for manual and non-manual reset.
        //       It would reduce late decision bools, but it would require creating
        //       generalized internal::event or adding new specialized
        //       internal::auto_reset_event. For current project state it is unnecessary
        //       complexity.
        volatile common::MemoryBuffer< Event, max_number> m_data{};
    };

    inline bool create(
        Context &    a_context,
        Id &         a_id,
        bool         a_manual_reset,
        const char * ap_name
    )
    {
        kernel::hardware::CriticalSection critical_section;

        // Create new Event object.
        MemoryBufferIndex new_item_id;

        if ( false == a_context.m_data.allocate( new_item_id))
        {
            return false;
        }

        a_id = static_cast< Id>( new_item_id);

        // Initialize new object.
        volatile Event & new_event = a_context.m_data.at( new_item_id);

        new_event.m_manual_reset = a_manual_reset;
        new_event.m_state = State::Reset;
        new_event.mp_name = ap_name;

        return true;
    }

    inline bool open( Context & a_context, Id & a_id, const char * ap_name)
    {
        assert( nullptr != ap_name);

        kernel::hardware::CriticalSection critical_section;

        for ( uint32_t id = 0U; id < max_number; ++id)
        {
            if ( true == a_context.m_data.isAllocated( static_cast< MemoryBufferIndex>( id)))
            {
                if ( ap_name == a_context.m_data.at( static_cast< MemoryBufferIndex>( id)).mp_name)
                {
                    a_id = static_cast< Id>( id);
                    return true;
                }
            }
        }

        return false;
    }

    inline void destroy( Context & a_context, Id & a_id)
    {
        kernel::hardware::CriticalSection critical_section;

        a_context.m_data.free( static_cast< MemoryBufferIndex>( a_id));
    }

    inline void set( Context & a_context, Id & a_id)
    {
        kernel::hardware::CriticalSection critical_section;

        a_context.m_data.at( static_cast< MemoryBufferIndex>( a_id)).m_state = State::Set;
    }

    inline void reset( Context & a_context, Id & a_id)
    {
        kernel::hardware::CriticalSection critical_section;

        a_context.m_data.at( static_cast< MemoryBufferIndex>( a_id)).m_state = State::Reset;
    }

    // Reset event state if manual reset is disabled.
    inline void manualReset( Context & a_context, Id & a_id)
    {
        kernel::hardware::CriticalSection critical_section;

        auto & event = a_context.m_data.at( static_cast< MemoryBufferIndex>( a_id));

        if ( false == event.m_manual_reset)
        {
            event.m_state = State::Reset;
        }
    }

    inline bool isSignaled( Context & a_context, Id & a_id)
    {
        kernel::hardware::CriticalSection critical_section;

        auto & event = a_context.m_data.at( static_cast< MemoryBufferIndex>( a_id));

        if ( internal::event::State::Set == event.m_state)
        {
            return true;
        }

        return false;
    }
}
