#pragma once

#include <kernel.hpp>

#include <atomic>

namespace kernel::internal::system_timer
{
    // Round-robin context switch intervals in miliseconds.
    constexpr Time_ms CONTEXT_SWITCH_INTERVAL_MS = 10U;

    struct Context
    {
        // Used to calculate round-robin context switch intervals.
        Time_ms m_oldTime = 0U;

        // Time in miliseconds elapsed since kernel started.
        volatile std::atomic<Time_ms> m_currentTime = 0U;
    };

    inline Time_ms get( Context & a_context)
    {
        return a_context.m_currentTime;
    }

    inline void increment( Context & a_context)
    {
        ++a_context.m_currentTime;
    }

    inline bool isIntervalElapsed( Context & a_context)
    {
        if (a_context.m_currentTime - a_context.m_oldTime > CONTEXT_SWITCH_INTERVAL_MS)
        {
            a_context.m_oldTime = a_context.m_currentTime;
            return true;
        }
        return false;
    }
};

