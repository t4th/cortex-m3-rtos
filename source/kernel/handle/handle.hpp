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
}
