#pragma once

#include <cstdint>

namespace kernel::handle
{
    enum class ObjectType
    {
        Task = 0,
        Timer,
        Event
    };
    
    uint32_t create(uint32_t index, ObjectType type);
    void close(uint32_t & handle);
    
    uint32_t getIndex(uint32_t handle);
    ObjectType getObjectType(uint32_t handle);
}
