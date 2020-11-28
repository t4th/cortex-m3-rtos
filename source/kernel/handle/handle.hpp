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
    
    uint32_t getIndex(kernel::Handle & a_handle);

    ObjectType getObjectType(kernel::Handle & a_handle);

    template<typename TId>
    TId getId(Handle & a_handle)
    {
        TId id;
        id.m_id = internal::handle::getIndex(a_handle);

        return id;
    }
}
