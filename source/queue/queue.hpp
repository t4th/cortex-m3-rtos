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
        
        size_t      m_data_max_size{ 0U};
        size_t      m_data_type_size{ 0U};
        uint8_t *   m_data{ nullptr};

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
        size_t &                    a_data_max_size,
        size_t &                    a_data_type_size,
        uint8_t &                   a_data
    )
    {
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
        
        new_queue.m_data_max_size = a_data_max_size;
        new_queue.m_data_type_size = a_data_type_size;
        new_queue.m_data = &a_data;
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

    inline bool isFull( Context & a_context, Id & a_id)
    {
        return ( a_context.m_data.at( a_id).m_current_size >= a_context.m_data.at( a_id).m_data_max_size);
    }

    inline bool isEmpty( Context & a_context, Id & a_id)
    {
        return ( 0U == a_context.m_data.at( a_id).m_current_size);
    }

    inline bool send(
        Context &                   a_context,
        internal::event::Context &  a_event_context,
        Id &                        a_id,
        uint8_t &                   a_data
    )
    {
        volatile Queue & queue = a_context.m_data.at( a_id);
        
        if ( true == isFull( a_context, a_id))
        {
            return false;
        }

        if ( false == isEmpty( a_context, a_id))
        {
            ++queue.m_head;
            
            if ( queue.m_head >= queue.m_data_max_size)
            {
                queue.m_head = 0U;
            }
        }

        // memory copy
        // todo: consider using normal memcpy
        {
            uint8_t * dst = queue.m_data;
            uint8_t * src = &a_data;

            size_t real_head_offset = queue.m_data_type_size * queue.m_head;
            dst = dst + real_head_offset;

            for ( size_t i = 0U; i < queue.m_data_type_size; ++i)
            {
                dst[ i] = src[ i];
            }
        }

        ++queue.m_current_size;

        return true;
    }
    
    inline bool receive(
        Context &                   a_context,
        internal::event::Context &  a_event_context,
        Id &                        a_id,
        uint8_t &                   a_data
    )
    {
        volatile Queue & queue = a_context.m_data.at( a_id);
        
        if ( true == isEmpty( a_context, a_id))
        {
            return false;
        }
                // memory copy
        {
            uint8_t * dst = &a_data;
            uint8_t * src = queue.m_data;
            size_t real_head_offset = queue.m_data_type_size * queue.m_head;
            src = src + real_head_offset;

            for ( size_t i = 0U; i < queue.m_data_type_size; ++i)
            {
                dst[ i] = src[ i];
            }
        }

        if ( queue.m_current_size > 1U)
        {
            ++queue.m_tail;

            if ( queue.m_tail >= queue.m_data_max_size)
            {
                queue.m_tail = 0U;
            }
        }

        --queue.m_current_size;
            
        return true;
    }

    namespace sync_event_id
    {
        inline internal::event::Id get( Context & a_context, Id & a_id)
        {
            return a_context.m_data.at( a_id).m_sync_event;
        }
    }
}
