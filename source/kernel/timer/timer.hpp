#pragma once

#include <kernel.hpp>
#include <memory_buffer.hpp>

namespace kernel::internal::timer
{
    constexpr uint32_t MAX_NUMBER = 16U;

    // todo: consider it type strong
    typedef uint32_t Id;

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
        volatile kernel::internal::common::MemoryBuffer<Timer, MAX_NUMBER> m_data;
    };

    bool create(
        Context &           a_context,
        Id &                a_id,
        Time_ms &           a_start,
        Time_ms &           a_interval,
        kernel::Handle *    a_signal = nullptr
    );

    inline void destroy( Context & a_context, Id & a_id)
    {
        a_context.m_data.free(a_id);
    }

    inline void start( Context & a_context, Id & a_id)
    {
        a_context.m_data.at(a_id).m_state = State::Started;
    }

    inline void stop( Context & a_context, Id & a_id)
    {
        a_context.m_data.at(a_id).m_state = State::Stopped;
    }

    inline State getState( Context & a_context, Id & a_id)
    {
        return a_context.m_data.at(a_id).m_state;
    }

    void tick( Context & a_context, Time_ms & current);
}
