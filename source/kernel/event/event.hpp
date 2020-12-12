#pragma once

#include <kernel.hpp>
#include <memory_buffer.hpp>

namespace kernel::internal::event
{
    constexpr uint32_t MAX_NUMBER = 32U;

    // todo: consider it type strong
    typedef uint32_t Id;

    enum class State
    {
        Reset,
        Set,
        // Note: this special state is used for 
        //       WaitForMultipleObjects, where
        //       all events that are being check
        //       are cleared when ALL events are
        //       set and then Updated.
        SetUpdateLater
    };

    struct Event
    {
        // todo: this needs to be interlocked
        State m_state;
        bool  m_manual_reset;
    };

    struct Context
    {
        volatile kernel::internal::common::MemoryBuffer<Event, MAX_NUMBER> m_data;
    };

    bool create(
        Context &   a_context,
        Id &        a_id,
        bool        a_manual_reset
    );

    inline void destroy( Context & a_context, Id & a_id)
    {
        a_context.m_data.free(a_id);
    }

    inline void set( Context & a_context, Id & a_id)
    {
        a_context.m_data.at(a_id).m_state = State::Set;
    }

    inline void reset( Context & a_context, Id & a_id)
    {
        a_context.m_data.at(a_id).m_state = State::Reset;
    }

    namespace state
    {
        // Reset all events than have manual reset disabled.
        // This function should be called after all events were signaled via
        // WaitForObject functions.
        inline void updateAll( Context & a_context)
        {
            for (Id i = 0U; i < MAX_NUMBER; ++i)
            {
                if (true == a_context.m_data.isAllocated(i))
                {
                    if (false == a_context.m_data.at(i).m_manual_reset)
                    {
                        if (State::SetUpdateLater == a_context.m_data.at(i).m_state)
                        {
                            reset(a_context, i);
                        }
                    }
                }
            }
        }

        inline State get( Context & a_context, Id & a_id)
        {
            State state = a_context.m_data.at(a_id).m_state;

            // TODO: possible memory barrier here

            if (false == a_context.m_data.at(a_id).m_manual_reset)
            {
                if (State::Set == state)
                {
                    a_context.m_data.at(a_id).m_state = State::SetUpdateLater;
                }
            }

            if (State::Reset != state)
            {
                state = State::Set;
            }

            return state;
        }
    }
}
