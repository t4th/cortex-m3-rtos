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
    
    kernel::Handle create(ObjectType a_type, uint32_t & a_index);
    
    ObjectType getObjectType(kernel::Handle & a_handle);

    // Strong typed version of getIndex.
    template<typename TId>
    TId getId(Handle & a_handle)
    {
        TId id;
        id.m_id = a_handle.m_data & 0xFFFFU;

        return id;
    }
}
