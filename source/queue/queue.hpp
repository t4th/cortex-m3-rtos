#pragma once

#include "config/config.hpp"
#include "common/memory_buffer.hpp"

#include <kernel.hpp>

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

        kernel::Handle * m_event{ nullptr};
    };

    struct Context
    {
        volatile kernel::internal::common::
            MemoryBuffer< Queue, max_number> m_data{};
    };

    inline bool create(
        Context &           a_context,
        Id &                a_id,
        size_t &            a_data_size,
        void * const        a_data,
        kernel::Handle &    a_event
    )
    {
        assert( nullptr != a_data);

        // Create new Queue object.
        uint32_t new_item_id;

        if ( false == a_context.m_data.allocate( new_item_id))
        {
            return false;
        }

        a_id = new_item_id;

        // Initialize new Queue object.
        volatile Queue & new_queue = a_context.m_data.at( new_item_id);
        
        new_queue.m_current_size = 0U;
        new_queue.m_head = 0U;
        new_queue.m_tail = 0U;
        
        new_queue.m_data_size = a_data_size;
        new_queue.m_data = a_data;

        new_queue.m_event = &a_event;

        return true;
    }

}
