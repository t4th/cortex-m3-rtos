#pragma once

#include "config/config.hpp"
#include "common/memory_buffer.hpp"
#include "common/memory.hpp"

#include "../kernel.hpp"

// Static Queue implementation.

// Queue main usage is data transfer between tasks and between tasks and hardware interrupts.
// This require usage of hardware level critical sections for context access.

namespace kernel::internal::queue
{
    // Type strong index of Queue.
    enum class Id : uint32_t{};

    struct Queue
    {
        size_t              m_current_size{ 0U};
        uint32_t            m_head{ 0U};
        uint32_t            m_tail{ 0U};
        
        size_t              m_data_max_elements{ 0U};
        size_t              m_data_type_size{ 0U};
        volatile uint8_t *  mp_data{ nullptr};

        const char *        mp_name{ nullptr};
    };

    // Type strong memory index for allocated Queue type.
    typedef common::MemoryBuffer< Queue, max_number>::Id MemoryBufferIndex;

    struct Context
    {
        volatile common::MemoryBuffer< Queue, max_number> m_data{};
    };

    inline bool create(
        Context &               a_context,
        Id &                    a_id,
        size_t &                a_data_max_elements,
        size_t &                a_data_type_size,
        volatile void * const   ap_static_buffer,
        const char *            ap_name
    )
    {
        assert( nullptr != ap_static_buffer);

        kernel::hardware::CriticalSection critical_section{ critical_section_priority};

        // Create new Queue object.
        MemoryBufferIndex new_queue_id;

        if ( false == a_context.m_data.allocate( new_queue_id))
        {
            return false;
        }

        a_id = static_cast< Id>( new_queue_id);

        volatile Queue & new_queue = a_context.m_data.at( new_queue_id);

        // Initialize new Queue object.
        new_queue.m_current_size = 0U;
        new_queue.m_head = 0U;
        new_queue.m_tail = 0U;
        
        new_queue.m_data_max_elements = a_data_max_elements;
        new_queue.m_data_type_size = a_data_type_size;
        new_queue.mp_data = reinterpret_cast< volatile uint8_t *>( ap_static_buffer);

        // TODO: consider checking for dublicates.
        new_queue.mp_name = ap_name;

        return true;
    }

    inline bool open( Context & a_context, Id & a_id, const char * ap_name)
    {
        assert( nullptr != ap_name);

        kernel::hardware::CriticalSection critical_section{ critical_section_priority};

        for ( uint32_t id = 0U; id < max_number; ++id)
        {
            if ( true == a_context.m_data.isAllocated( static_cast< MemoryBufferIndex>( id)))
            {
                if ( ap_name == a_context.m_data.at( static_cast< MemoryBufferIndex>( id)).mp_name)
                {
                    a_id = static_cast< Id>( id);
                    return true;
                }
            }
        }

        return false;
    }

    inline void destroy( Context & a_context, Id & a_id)
    {
        kernel::hardware::CriticalSection critical_section{ critical_section_priority};

        a_context.m_data.free( static_cast< MemoryBufferIndex>( a_id));
    }

    inline bool isFull( Context & a_context, Id & a_id)
    {
        kernel::hardware::CriticalSection critical_section{ critical_section_priority};

        volatile Queue & queue = a_context.m_data.at( static_cast< MemoryBufferIndex>( a_id));

        bool is_queue_full = ( queue.m_current_size >= queue.m_data_max_elements);

        return is_queue_full;
    }

    inline bool isEmpty( Context & a_context, Id & a_id)
    {
        kernel::hardware::CriticalSection critical_section{ critical_section_priority};

        bool is_queue_empty = ( 0U == a_context.m_data.at( static_cast< MemoryBufferIndex>( a_id)).m_current_size);

        return is_queue_empty;
    }

    // Push item to the head.
    inline bool send( Context & a_context, Id & a_id, volatile void * const ap_data)
    {
        assert( nullptr != ap_data);

        kernel::hardware::CriticalSection critical_section{ critical_section_priority};

        volatile Queue & queue = a_context.m_data.at( static_cast< MemoryBufferIndex>( a_id));

        if ( true == isFull( a_context, a_id))
        {
            return false;
        }

        if ( false == isEmpty( a_context, a_id))
        {
            ++queue.m_head;

            if ( queue.m_head >= queue.m_data_max_elements)
            {
                queue.m_head = 0U;
            }
        }

        // Memory copy.
        // todo: consider memory fence here.
        {
            const size_t real_head_offset = queue.m_data_type_size * queue.m_head;

            volatile uint8_t & destination = *( queue.mp_data + real_head_offset);
            auto & source = *reinterpret_cast < const volatile uint8_t *>( ap_data);

            memory::copy( destination, source, queue.m_data_type_size);
        }

        ++queue.m_current_size;

        return true;
    }
    
    // Pop item from the tail.
    inline bool receive( Context & a_context, Id & a_id, volatile void * const ap_data)
    {
        assert( nullptr != ap_data);

        kernel::hardware::CriticalSection critical_section{ critical_section_priority};

        volatile Queue & queue = a_context.m_data.at( static_cast< MemoryBufferIndex>( a_id));

        if ( true == isEmpty( a_context, a_id))
        {
            return false;
        }

        // Memory copy.
        // todo: consider memory fence here.
        {
            size_t real_tail_offset = queue.m_data_type_size * queue.m_tail;

            auto & destination = *reinterpret_cast < volatile uint8_t *>( ap_data);
            const volatile uint8_t & source = *( queue.mp_data + real_tail_offset);

            memory::copy( destination, source, queue.m_data_type_size);
        }

        if ( queue.m_current_size > 1U)
        {
            ++queue.m_tail;

            if ( queue.m_tail >= queue.m_data_max_elements)
            {
                queue.m_tail = 0U;
            }
        }

        --queue.m_current_size;

        return true;
    }

    inline size_t getSize( Context & a_context, Id & a_id)
    {
        kernel::hardware::CriticalSection critical_section{ critical_section_priority};

        return a_context.m_data.at( static_cast< MemoryBufferIndex>( a_id)).m_current_size;
    }
}
