#include "config/config.hpp"

// Timer used to calculate round-robin context switch intervals.
namespace kernel::internal::system_timer
{
    struct Context
    {
        // Previous tick time in miliseconds.
        volatile TimeMs m_oldTime = 0U;

        // Time in miliseconds elapsed since kernel started.
        volatile std::atomic< TimeMs> m_currentTime = 0U;
    };

    inline TimeMs get( Context & a_context)
    {
        return a_context.m_currentTime;
    }

    inline void increment( Context & a_context)
    {
        ++a_context.m_currentTime;
    }

    inline bool isIntervalElapsed( Context & a_context)
    {
        if ( a_context.m_currentTime - a_context.m_oldTime > context_switch_interval_ms)
        {
            a_context.m_oldTime = a_context.m_currentTime;
            return true;
        }

        return false;
    }
};

