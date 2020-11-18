#include <timer.hpp>

namespace kernel::internal::timer
{
    bool create(
        Context &           a_context,
        Id &                a_id,
        Time_ms             a_interval,
        kernel::Handle *    a_signal
    )
    {
        // Create new Timer object.
        uint32_t new_item_id;

        if (false == a_context.m_data.allocate(new_item_id))
        {
            return false;
        }

        a_id.m_id = new_item_id;

        // Initialize new Timer object.
        Timer & new_timer = a_context.m_data.at(new_item_id);

        new_timer.m_start = kernel::getTime();
        new_timer.m_interval = a_interval;
        new_timer.m_signal = a_signal;

        new_timer.m_state = State::Stopped;

        return true;
    }

    void destroy( Context & a_context, Id a_id)
    {
        a_context.m_data.free(a_id.m_id);
    }

    void start( Context & a_context, Id a_id)
    {
        a_context.m_data.at(a_id.m_id).m_state = State::Started;
    }

    void stop( Context & a_context, Id a_id)
    {
        a_context.m_data.at(a_id.m_id).m_state = State::Stopped;
    }

    void runTimers( Context & a_context)
    {
        for (uint32_t i = 0U; i < MAX_TIMER_NUMBER; ++i)
        {
            if (a_context.m_data.isAllocated(i))
            {
                Timer & timer = a_context.m_data.at(i);

                if (State::Started == timer.m_state)
                {
                    Time_ms current = kernel::getTime();

                    if ((current - timer.m_start) > timer.m_interval)
                    {
                        timer.m_state = State::Finished;

                        // todo: get handle type and do stuff
                        // resume task
                        // set event
                    }
                }
            }
        }
    }
}
