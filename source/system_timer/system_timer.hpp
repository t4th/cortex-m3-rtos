#include "config/config.hpp"

#ifndef __GNUC__
    // Workaround for nano lib issues with GCC and atomic
    #include <atomic>
#endif

// Timer used to calculate round-robin context switch intervals.
namespace kernel::internal::system_timer
{
    struct Context
    {
        // Previous tick time in miliseconds.
        volatile TimeMs m_old_time{ 0U};

        // Time in miliseconds elapsed since kernel started.
        #ifndef __GNUC__
            std::atomic< TimeMs> m_current_time{ 0U};
        #else
            volatile TimeMs m_current_time{ 0U };
        #endif

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
        const auto diff_ms{ a_context.m_current_time - a_context.m_old_time};

        if ( diff_ms > context_switch_interval_ms)
        {
            a_context.m_old_time = a_context.m_current_time;
            return true;
        }

        return false;
    }
};

