#pragma once

#include <kernel.hpp>
#include <memory_buffer.hpp>

namespace kernel::internal::event
{
    constexpr uint32_t MAX_NUMBER = 16U;

    typedef struct
    {
        uint32_t m_id;
    } Id;

    enum class State
    {
        Reset,
        Set
    };

    struct Event
    {
        State m_state;
        bool  m_manual_reset;
    };

    struct Context
    {
        kernel::internal::common::MemoryBuffer<Event, MAX_NUMBER> m_data;
    };

    bool create(
        Context &   a_context,
        Id &        a_id,
        bool        a_manual_reset
    );

    void destroy( Context & a_context, Id & a_id);

    void set( Context & a_context, Id & a_id);

    void reset( Context & a_context, Id & a_id);
}
