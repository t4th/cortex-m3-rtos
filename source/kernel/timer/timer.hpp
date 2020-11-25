#pragma once

#include <kernel.hpp>
#include <memory_buffer.hpp>

namespace kernel::internal::timer
{
    constexpr uint32_t MAX_NUMBER = 16U;

    typedef struct
    {
        uint32_t m_id;
    } Id; // Internal timer ID.

    enum class State
    {
        Stopped, // Timer created.
        Started, // Timer started.
        Finished // Timer reached required interval.
    };

    struct Timer
    {
        Time_ms             m_start;
        Time_ms             m_interval;
        State               m_state;
        kernel::Handle *    m_signal;
    };

    struct Context
    {
        kernel::internal::common::MemoryBuffer<Timer, MAX_NUMBER> m_data;
    };

    bool create(
        Context &           a_context,
        Id &                a_id,
        Time_ms             a_interval,
        kernel::Handle *    a_signal = nullptr
    );

    void destroy( Context & a_context, Id a_id);

    void start( Context & a_context, Id a_id);

    void stop( Context & a_context, Id a_id);

    void runTimers( Context & a_context);
}
