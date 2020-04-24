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
        struct Context
        {
            std::array<TDataType, MaxSize>  m_data;
            std::array<bool, MaxSize>       m_status; // TODO: Is bool 1 byte or 4? Hack it to 1-bit - 1-status.

            Context() : m_data{}, m_status{} {}
        };

        MemoryBuffer(Context & a_context)
            : m_context(a_context)
        {}
            
        bool allocate(uint32_t & a_item_id)
        {
            // Find first not used slot and return index as ID.
            for (uint32_t i = 0; i < MaxSize; ++i)
            {
                if (false == m_context.m_status[i])
                {
                    m_context.m_status[i] = true;
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
                m_context.m_status[a_item_id] = false;
            }
        }

        TDataType & at(uint32_t a_item_id)
        {
            // TODO: Make it safe. For now, implementation assumes input data is
            // always valid since this is used only by kernel modules.

            return m_context.m_data[a_item_id];
        }
    private:
        Context & m_context;
    };
}
