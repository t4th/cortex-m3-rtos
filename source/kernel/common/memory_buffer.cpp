#include <memory_buffer.hpp>

namespace kernel::common
{
    template <typename TDataType, std::size_t MaxSize>
    bool MemoryBuffer<TDataType, MaxSize>::allocate(uint32_t & a_item_id)
    {
        // Find first not used slot and return index as ID.
        for (uint32_t i = 0; i < MaxSize; ++i)
        {
            if (false == m_status[i])
            {
                m_status[i] = true;
                a_item_id = i;
                return true;
            }
        }
        
        return false;
    }
    
    template <typename TDataType, std::size_t MaxSize>
    void MemoryBuffer<TDataType, MaxSize>::free(uint32_t & a_item_id)
    {
        if (a_item_id < MaxSize)
        {
            m_status[a_item_id] = false;
        }
    }
    
    template <typename TDataType, std::size_t MaxSize>
    TDataType & MemoryBuffer<TDataType, MaxSize>::get(uint32_t & a_item_id)
    {
        // TODO: Make it safe. For now, implementation assumes input data is
        // always valid since this is used only by kernel modules.
        
        return m_data[a_item_id];
    }
}