#pragma once

#include <cassert>
#include <hardware.hpp>

namespace kernel::internal::common
{
    // Abstract memory buffer. Idea behind it is to keep data and status separetly as
    // arrays of structs.
    template < typename TDataType, std::size_t MaxSize>
    class MemoryBuffer
    {
    public:
        // Note: m_data is not initialized by design.
        MemoryBuffer() : m_status{} {}
            
        inline bool allocate( uint32_t & a_item_id) volatile
        {
            // Find first not used slot and return index as ID.
            for ( uint32_t i = 0U; i < MaxSize; ++i)
            {
                if ( false == m_status[ i])
                {
                    m_status[ i] = true;
                    a_item_id = i;
                    return true;
                }
            }

            return false;
        }

        inline void free( uint32_t a_item_id) volatile
        {
            assert( a_item_id < MaxSize);

            m_status[ a_item_id] = false;
        }

        inline void freeAll() volatile
        {
            for ( uint32_t i = 0U; i < MaxSize; ++i)
            {
                m_status[ i] = false;
            }
        }

        inline volatile TDataType & at( uint32_t a_item_id) volatile
        {
            assert( a_item_id < MaxSize);
            assert( m_status[ a_item_id]);

            return m_data[ a_item_id];
        }

        inline bool isAllocated( uint32_t a_item_id) volatile
        {
            assert( a_item_id < MaxSize);

            return m_status[ a_item_id];
        }

    private:
        TDataType   m_data[ MaxSize];

        // TODO: Is bool 1 byte or 4? Hack it to 1-bit - 1-status.
        bool        m_status[ MaxSize];

    };
}
