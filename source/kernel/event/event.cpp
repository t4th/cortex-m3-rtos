#include <event.hpp>

namespace kernel::internal::event
{
    bool create(
        Context &   a_context,
        Id &        a_id,
        bool        a_manual_reset
    )
    {
        // Create new Event object.
        uint32_t new_item_id;

        if (false == a_context.m_data.allocate(new_item_id))
        {
            return false;
        }

        a_id = new_item_id;

        // Initialize new object.
        volatile Event & new_event = a_context.m_data.at(new_item_id);

        new_event.m_manual_reset = a_manual_reset;
        new_event.m_state = State::Reset;

        return true;
    }

    State getState( Context & a_context, Id & a_id)
    {
        volatile Event & event = a_context.m_data.at(a_id);
        State state = event.m_state;

        if (false == event.m_manual_reset)
        {
            event.m_state = State::Reset;
        }

        return state;
    }
}
