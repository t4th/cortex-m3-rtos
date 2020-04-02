#pragma once

#include <array>

namespace kernel::common
{
    // Abstract memory buffer. Idea behind it is to keep data and status separetly as
    // array of structs data structure.
    template <typename TDataType, std::size_t MaxSize>
    class MemoryBuffer
    {
        private:
            std::array<TDataType, MaxSize>  m_data;
            std::array<bool, MaxSize>       m_status; // TODO: Is bool 1 byte or 4? Hack it to 1-bit - 1-status.
        public:
            bool allocate(uint32_t & a_item_id);
            void free(uint32_t & a_item_id);
            TDataType & get(uint32_t & a_item_id);
    };
}
