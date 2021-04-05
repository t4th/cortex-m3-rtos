#pragma once

#include <cassert>

namespace kernel::internal::common
{
    // Abstract memory buffer. Idea behind it is to keep data and status separetly as
    // an arrays of structs.
    template < typename TDataType, std::size_t MaxSize>
    class MemoryBuffer
    {
    public:
        // Type strong index of allocated memory block.
        enum class Id : uint32_t{};

        // Note: m_data is not initialized by design.
        MemoryBuffer() : m_status{} {}
            
        inline bool allocate( Id & a_item_id) volatile
        {
            // Find first not used slot and return index as ID.
            for ( uint32_t i = 0U; i < MaxSize; ++i)
            {
                if ( false == m_status[ i])
                {
                    m_status[ i] = true;
                    a_item_id = static_cast< Id>( i);
                    return true;
                }
            }

            return false;
        }

        inline void free( Id a_item_id) volatile
        {
            assert( static_cast< uint32_t>( a_item_id) < MaxSize);

            m_status[ static_cast< uint32_t>( a_item_id)] = false;
        }

        inline void freeAll() volatile
        {
            for ( uint32_t i = 0U; i < MaxSize; ++i)
            {
                m_status[ i] = false;
            }
        }

        inline volatile TDataType & at( Id a_item_id) volatile
        {
            assert( static_cast< uint32_t>( a_item_id) < MaxSize);
            assert( m_status[ static_cast< uint32_t>( a_item_id)]);

            return m_data[ static_cast< uint32_t>( a_item_id)];
        }

        inline bool isAllocated( Id a_item_id) volatile
        {
            assert( static_cast< uint32_t>( a_item_id) < MaxSize);

            return m_status[ static_cast< uint32_t>( a_item_id)];
        }

    private:
        TDataType   m_data[ MaxSize];
        
        // Note: This is not effective way to hold information, but simple enough
        //       for this project.
        bool        m_status[ MaxSize];

    };
}
