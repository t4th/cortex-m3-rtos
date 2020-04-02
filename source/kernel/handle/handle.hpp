#pragma once

#include <cstdint>

namespace kernel::handle
{
    enum class ObjectType : uint32_t
    {
        task = 0,
        timer,
        event
    };
    
    uint32_t create(uint32_t index, ObjectType type);
    void close(uint32_t & handle);
    
    uint32_t getIndex(uint32_t handle);
    ObjectType getObjectType(uint32_t handle);
}
