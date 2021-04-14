#include "config/config.hpp"

// Timer used to calculate round-robin context switch intervals.
namespace kernel::internal::system_timer
{
    struct Context
    {
        // Previous tick time in miliseconds.
        volatile TimeMs m_old_time{ 0U};

        // Time in miliseconds elapsed since kernel started.
        volatile std::atomic< TimeMs> m_current_time{ 0U};
    };

    inline TimeMs get( Context & a_context)
    {
        return a_context.m_current_time;
    }

    inline void increment( Context & a_context)
    {
        ++a_context.m_current_time;
    }

    inline bool isIntervalElapsed( Context & a_context)
    {
        if ( a_context.m_current_time - a_context.m_old_time > context_switch_interval_ms)
        {
            a_context.m_old_time = a_context.m_current_time;
            return true;
        }

        return false;
    }
};

