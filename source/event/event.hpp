#pragma once

#include "config/config.hpp"
#include "common/memory_buffer.hpp"

#include <kernel.hpp>

namespace kernel::internal::event
{
    // todo: consider it type strong
    typedef uint32_t Id;

    enum class State
    {
        Reset,
        Set
    };

    struct Event
    {
        State m_state;
        bool  m_manual_reset;
    };

    struct Context
    {
        // Note: I considered creating two lists: for manual and non-manual reset.
        //       It would reduce late decision bools, but it would require creating
        //       generalized internal::event or adding new specialized
        //       internal::auto_reset_event. For current project state it is unnecessary
        //       complexity.
        volatile kernel::internal::common::MemoryBuffer< Event, max_number> m_data{};
    };

    inline bool create(
        Context &   a_context,
        Id &        a_id,
        bool        a_manual_reset
    )
    {
        // Create new Event object.
        uint32_t new_item_id;

        if ( false == a_context.m_data.allocate( new_item_id))
        {
            return false;
        }

        a_id = new_item_id;

        // Initialize new object.
        volatile Event & new_event = a_context.m_data.at( new_item_id);

        new_event.m_manual_reset = a_manual_reset;
        new_event.m_state = State::Reset;

        return true;
    }

    inline void destroy( Context & a_context, Id & a_id)
    {
        a_context.m_data.free( a_id);
    }

    inline void set( Context & a_context, Id & a_id)
    {
        a_context.m_data.at( a_id).m_state = State::Set;
    }

    inline void reset( Context & a_context, Id & a_id)
    {
        a_context.m_data.at( a_id).m_state = State::Reset;
    }

    // Reset event state if manual reset is disabled.
    inline void manualReset( Context & a_context, Id & a_id)
    {
        auto & event = a_context.m_data.at( a_id);

        if ( false == event.m_manual_reset)
        {
            event.m_state = State::Reset;
        }
    }

    inline bool isSignaled( Context & a_context, Id & a_id)
    {
        auto & event = a_context.m_data.at( a_id);

        if ( internal::event::State::Set == event.m_state)
        {
            return true;
        }

        return false;
    }
}
