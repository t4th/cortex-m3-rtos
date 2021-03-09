#pragma once

#include "config/config.hpp"
#include "common/memory_buffer.hpp"

#include <kernel.hpp>

#include <event/event.hpp>

// Static Queue implementation.
// User is providing pointer to data and data size.
// Queue is managing life cycle of synchronization event so
// queue handle could be used with WaitForObject functions.

namespace kernel::internal::queue
{
    // todo: consider it type strong
    typedef uint32_t Id;

    constexpr size_t max_number{ 5U};

    struct Queue
    {
        size_t      m_current_size{ 0U};
        uint32_t    m_head{ 0U};
        uint32_t    m_tail{ 0U};

        size_t      m_data_size{ 0U};
        void *      m_data{ nullptr};

        kernel::internal::event::Id m_sync_event{ 0U};
    };

    struct Context
    {
        volatile kernel::internal::common::
            MemoryBuffer< Queue, max_number> m_data{};
    };

    inline bool create(
        Context &                   a_context,
        internal::event::Context &  a_event_context,
        Id &                        a_id,
        size_t &                    a_data_size,
        void * const                a_data
    )
    {
        assert( nullptr != a_data);

        // Create new Queue object.
        uint32_t new_queue_id;

        if ( false == a_context.m_data.allocate( new_queue_id))
        {
            return false;
        }

        a_id = new_queue_id;

        // create synchronization event
        kernel::internal::event::Id event_id;

        bool event_created = internal::event::create(
            a_event_context,
            event_id,
            false
            );

        if ( false == event_created)
        {
            a_context.m_data.free( new_queue_id);
            return false;
        }

        volatile Queue & new_queue = a_context.m_data.at( new_queue_id);

        // Initialize new Queue object.
        new_queue.m_current_size = 0U;
        new_queue.m_head = 0U;
        new_queue.m_tail = 0U;
        
        new_queue.m_data_size = a_data_size;
        new_queue.m_data = a_data;
        new_queue.m_sync_event = event_id;

        return true;
    }

    inline void destroy(
        Context &                   a_context,
        internal::event::Context    a_event_context,
        Id &                        a_id
    )
    {
        // destroy event
        internal::event::Id event_id = a_context.m_data.at( a_id).m_sync_event;
        internal::event::destroy( a_event_context, event_id);
        
        // destroy queue
        a_context.m_data.free( a_id);
    }

    namespace sync_event_id
    {
        inline internal::event::Id get( Context & a_context, Id & a_id)
        {
            return a_context.m_data.at( a_id).m_sync_event;
        }
    }
}
