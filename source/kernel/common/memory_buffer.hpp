#pragma once

#include <array>

namespace kernel::common
{
    // Abstract memory buffer. Idea behind it is to keep data and status separetly as
    // arrays of structs.
    template <typename TDataType, std::size_t MaxSize>
    class MemoryBuffer
    {
    public:
        MemoryBuffer() : m_data{}, m_status{} {}
            
        bool allocate(uint32_t & a_item_id)
        {
            // Find first not used slot and return index as ID.
            for (uint32_t i = 0U; i < MaxSize; ++i)
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

        void free(uint32_t a_item_id)
        {
            if (a_item_id < MaxSize)
            {
                m_status[a_item_id] = false;
            }
        }

        TDataType & at(uint32_t a_item_id)
        {
            // TODO: Make it safe. For now, implementation assumes input data is
            // always valid since this is used only by kernel modules.

            return m_data[a_item_id];
        }

    private:
        std::array<TDataType, MaxSize>  m_data;
        std::array<bool, MaxSize>       m_status; // TODO: Is bool 1 byte or 4? Hack it to 1-bit - 1-status.

    };
}
