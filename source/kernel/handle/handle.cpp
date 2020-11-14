#include <handle.hpp>

namespace kernel::internal::handle
{
    kernel::Handle create(ObjectType a_type, uint32_t & a_index)
    {
        uint32_t new_handle = (static_cast<uint32_t>(a_type) << 16) | (a_index & 0xFFFF);
        
        return {new_handle};
    }
    
    uint32_t getIndex(kernel::Handle & a_handle)
    {
        return a_handle.m_data & 0xFFFF;
    }
    
    ObjectType getObjectType(kernel::Handle & a_handle)
    {
        uint32_t object_type = (a_handle.m_data >> 16) & 0xFFFF;
        
        return static_cast<ObjectType>(object_type);
    }
}
