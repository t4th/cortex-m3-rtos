#include <handle.hpp>

namespace kernel::handle
{
    uint32_t create(uint32_t index, ObjectType type)
    {
        uint32_t new_handle = (static_cast<uint32_t>(type) << 16) | (index & 0xFFFF);
        
        return new_handle;
    }
    
    void close(uint32_t & handle)
    {
        
    }
    
    uint32_t getIndex(uint32_t handle)
    {
        return handle & 0xFFFF;
    }
    
    ObjectType getObjectType(uint32_t handle)
    {
        uint32_t object_type = (handle >> 16) & 0xFFFF;
        
        return static_cast<ObjectType>(object_type);
    }
}
