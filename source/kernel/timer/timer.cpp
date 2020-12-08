#include <timer.hpp>

#include <handle.hpp>

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

        a_id = new_item_id;

        // Initialize new Timer object.
        volatile Timer & new_timer = a_context.m_data.at(new_item_id);

        new_timer.m_start = kernel::getTime();
        new_timer.m_interval = a_interval;
        new_timer.m_signal = a_signal;

        new_timer.m_state = State::Stopped;

        return true;
    }

    void tick( Context & a_context, Time_ms & a_current)
    {
        // TODO: consider memory barrier since this funtion is
        //       called from interrupt handler.

        // TODO: Iterate through all timers, even not allocated.
        //       Limiting branches should be more effective that
        //       late decision trees (and more cache friendly if
        //       applicable).
        for (uint32_t i = 0U; i < MAX_NUMBER; ++i)
        {
            // Note: For now this check stays due to memory buffer
            //       assert isAllocated when 'at' dereference is used.
            if (true == a_context.m_data.isAllocated(i))
            {
                volatile Timer & timer = a_context.m_data.at(i);

                if ((a_current - timer.m_start) > timer.m_interval)
                {
                    // TODO: Call the callback signal.
                    //       It would be more effective to make 2
                    //       buffers for each timer state.
                    timer.m_state = State::Finished;
                }
            }
        }
    }
}
