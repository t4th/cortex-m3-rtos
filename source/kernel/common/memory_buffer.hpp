#pragma once

#include <array>
#include <cassert>

namespace kernel::internal::common
{
    // Abstract memory buffer. Idea behind it is to keep data and status separetly as
    // arrays of structs.
    template <typename TDataType, std::size_t MaxSize>
    class MemoryBuffer
    {
    public:
        MemoryBuffer() : m_data{}, m_status{} {}
            
        bool allocate(uint32_t & a_item_id) volatile
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

        void free(uint32_t a_item_id) volatile
        {
            if (a_item_id < MaxSize)
            {
                m_status[a_item_id] = false;
            }
        }

        void freeAll() volatile
        {
            for (uint32_t i = 0U; i < MaxSize; ++i)
            {
                m_status[i] = false;
            }
        }

        volatile TDataType & at(uint32_t a_item_id) volatile
        {
            assert(a_item_id < MaxSize);
            assert(m_status[a_item_id]);

            return m_data[a_item_id];
        }

        bool isAllocated(uint32_t a_item_id) volatile
        {
            assert(a_item_id < MaxSize);

            return m_status[a_item_id];
        }

    private:
        TDataType   m_data[MaxSize];
        bool        m_status[MaxSize]; // TODO: Is bool 1 byte or 4? Hack it to 1-bit - 1-status.

    };
}
