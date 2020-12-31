#pragma once

#include <atomic>

// Kernel level critical section between thread and handler modes.
// todo: re-work
namespace kernel::internal::lock
{
    struct Context
    {
        volatile std::atomic< uint32_t> m_interlock = 0U;
    };

    inline bool isLocked( Context & a_context)
    {
        return ( 0U == a_context.m_interlock);
    }
    inline void enter( Context & a_context)
    {
        ++a_context.m_interlock;
    }
    inline void leave( Context & a_context)
    {
        --a_context.m_interlock;
    }
}
