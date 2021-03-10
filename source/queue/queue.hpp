#pragma once

#include "config/config.hpp"
#include "common/memory_buffer.hpp"

#include <kernel.hpp>

// Static Queue implementation.
// User is providing pointer to data and data size.

// Queue main usage is data transfer between tasks and between
// tasks and hardware interrupts. This require usage of hardware
// level critical sections for context access.

namespace kernel::internal::queue
{
    // todo: consider it type strong
    typedef uint32_t Id;

    struct Queue
    {
        size_t      m_current_size{ 0U};
        uint32_t    m_head{ 0U};
        uint32_t    m_tail{ 0U};
        
        size_t      m_data_max_size{ 0U};
        size_t      m_data_type_size{ 0U};
        uint8_t *   m_data{ nullptr};
    };

    struct Context
    {
        volatile kernel::internal::common::
            MemoryBuffer< Queue, max_number> m_data{};
    };

    inline bool create(
        Context &   a_context,
        Id &        a_id,
        size_t &    a_data_max_size,
        size_t &    a_data_type_size,
        uint8_t &   a_data
    )
    {
        // Create new Queue object.
        uint32_t new_queue_id;

        if ( false == a_context.m_data.allocate( new_queue_id))
        {
            return false;
        }

        a_id = new_queue_id;

        volatile Queue & new_queue = a_context.m_data.at( new_queue_id);

        // Initialize new Queue object.
        new_queue.m_current_size = 0U;
        new_queue.m_head = 0U;
        new_queue.m_tail = 0U;
        
        new_queue.m_data_max_size = a_data_max_size;
        new_queue.m_data_type_size = a_data_type_size;
        new_queue.m_data = &a_data;

        return true;
    }

    inline void destroy( Context & a_context, Id & a_id)
    {
        a_context.m_data.free( a_id);
    }

    inline bool isFull( Context & a_context, Id & a_id)
    {
        volatile Queue & queue = a_context.m_data.at( a_id);

        bool is_queue_full = ( queue.m_current_size >= queue.m_data_max_size);

        return is_queue_full;
    }

    inline bool isEmpty( Context & a_context, Id & a_id)
    {
        bool is_queue_empty = ( 0U == a_context.m_data.at( a_id).m_current_size);

        return is_queue_empty;
    }

    // Push item to the head.
    inline bool send(
        Context &   a_context,
        Id &        a_id,
        uint8_t &   a_data
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

        // Memory copy.
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
    
    // Pop item from the tail.
    inline bool receive(
        Context &   a_context,
        Id &        a_id,
        uint8_t &   a_data
    )
    {
        volatile Queue & queue = a_context.m_data.at( a_id);

        if ( true == isEmpty( a_context, a_id))
        {
            return false;
        }

        // Memory copy.
        // todo: consider using normal memcpy
        {
            uint8_t * dst = &a_data;
            uint8_t * src = queue.m_data;
            size_t real_tail_offset = queue.m_data_type_size * queue.m_tail;
            src = src + real_tail_offset;

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

    inline size_t getSize( Context & a_context, Id & a_id)
    {
        return a_context.m_data.at( a_id).m_current_size;
    }
}
