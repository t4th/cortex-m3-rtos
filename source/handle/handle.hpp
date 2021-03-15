#pragma once

#include "timer/timer.hpp"
#include "event/event.hpp"
#include "queue/queue.hpp"

#include <kernel.hpp>

// Handle is abstract type holding information of object type
// and object ID. 16 bit each in 32 bit variable.
namespace kernel::internal::handle
{
    enum class ObjectType
    {
        Task = 0,
        Timer,
        Event,
        Queue
    };
    
    inline kernel::Handle create( ObjectType a_type, uint32_t a_index)
    {
        return reinterpret_cast< kernel::Handle>(( static_cast< uint32_t>( a_type) << 16U) | ( a_index & 0xFFFFU));
    }

    inline ObjectType getObjectType( volatile kernel::Handle & a_handle)
    {
        uint32_t object_type = ( reinterpret_cast< uint32_t>( a_handle) >> 16U) & 0xFFFFU;

        return static_cast< ObjectType>( object_type);
    }

    // Strong typed version of getIndex.
    template< typename TId>
    inline TId getId( volatile Handle & a_handle)
    {
        return reinterpret_cast< uint32_t>( a_handle) & 0xFFFFU;
    }

    // Test if system object pointed by handle is in signaled state.
    // Return value indicate if Handle type is supported.
    inline bool testCondition(
        internal::timer::Context &  a_timer_context,
        internal::event::Context &  a_event_context,
        internal::queue::Context &  a_queue_context,
        volatile kernel::Handle &   a_handle,
        bool &                      a_condition_fulfilled
    )
    {
        const auto objectType = internal::handle::getObjectType( a_handle);

        a_condition_fulfilled = false;

        switch ( objectType)
        {
        case internal::handle::ObjectType::Event:
        {
            auto event_id = internal::handle::getId< internal::event::Id>( a_handle);
            a_condition_fulfilled = internal::event::isSignaled( a_event_context, event_id);
            break;
        }
        case internal::handle::ObjectType::Timer:
        {
            auto timer_id = internal::handle::getId< internal::timer::Id>( a_handle);
            auto timerState = internal::timer::getState( a_timer_context, timer_id);
            if ( internal::timer::State::Finished == timerState)
            {
                a_condition_fulfilled = true;
            }
            break;
        }
        // Signal task if queue is not empty.
        case internal::handle::ObjectType::Queue:
        {
            auto queue_id = internal::handle::getId< internal::queue::Id>( a_handle);
            bool is_queue_empty = internal::queue::isEmpty( a_queue_context, queue_id);
            if ( false == is_queue_empty)
            {
                a_condition_fulfilled = true;
            }
            break;
        }
        default:
        {
            return false;
        }
        };

        return true;
    }

    // Reset state of system object pointed by provided handle.
    inline void resetState(
        internal::event::Context &  a_event_context,
        volatile kernel::Handle &   a_handle
    )
    {
        const auto objectType = internal::handle::getObjectType( a_handle);

        switch ( objectType)
        {
        case internal::handle::ObjectType::Event:
        {
            auto event_id = internal::handle::getId< internal::event::Id>( a_handle);
            internal::event::manualReset( a_event_context, event_id);
            break;
        }
        default:
            break;
        };
    }
}
