#pragma once

#include <kernel.hpp>

namespace kernel::internal::handle
{
    enum class ObjectType
    {
        Task = 0,
        Timer,
        Event
    };
    
    inline kernel::Handle create(ObjectType a_type, uint32_t a_index)
    {
        return reinterpret_cast<kernel::Handle>((static_cast<uint32_t>(a_type) << 16U) | (a_index & 0xFFFFU));
    }

    inline ObjectType getObjectType(volatile kernel::Handle & a_handle)
    {
        uint32_t object_type = (reinterpret_cast<uint32_t>(a_handle) >> 16U) & 0xFFFFU;

        return static_cast<ObjectType>(object_type);
    }

    // Strong typed version of getIndex.
    template<typename TId>
    inline TId getId(volatile Handle & a_handle)
    {
        return reinterpret_cast<uint32_t>(a_handle) & 0xFFFFU;
    }

    // Return value indicate if Handle type was valid for condition check.
    inline bool testCondition(
        internal::timer::Context &  a_timer_context,
        internal::event::Context &  a_event_context,
        volatile kernel::Handle &   a_handle,
        bool &                      a_condition_fulfilled
    )
    {
        const auto objectType = internal::handle::getObjectType(a_handle);

        a_condition_fulfilled = false;

        switch (objectType)
        {
        case internal::handle::ObjectType::Event:
        {
            auto event_id = internal::handle::getId<internal::event::Id>(a_handle);
            auto evState = internal::event::state::get(a_event_context, event_id);
            if (internal::event::State::Set == evState)
            {
                a_condition_fulfilled = true;
            }
            break;
        }
        case internal::handle::ObjectType::Timer:
        {
            auto timer_id = internal::handle::getId<internal::timer::Id>(a_handle);
            auto timerState = internal::timer::getState(a_timer_context, timer_id);
            if (internal::timer::State::Finished == timerState)
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
}
