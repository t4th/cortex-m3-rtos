#include <handle.hpp>

namespace kernel::internal::handle
{
    kernel::Handle create(ObjectType a_type, uint32_t & a_index)
    {
        uint32_t new_handle = (static_cast<uint32_t>(a_type) << 16U) | (a_index & 0xFFFFU);
        
        return {new_handle};
    }
    
    ObjectType getObjectType(kernel::Handle & a_handle)
    {
        uint32_t object_type = (a_handle.m_data >> 16U) & 0xFFFFU;
        
        return static_cast<ObjectType>(object_type);
    }
}
