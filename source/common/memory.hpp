#pragma once

#include <cstdint>
#include <cassert>

// Note: I just didn't want to include whole <cstring>. 
//       Also references are so much nicer.
// TODO: Simplify, because code size is too big.
namespace kernel::internal::memory
{
    template<
        typename TTypeLeft,
        typename TTypeRight
    >
    inline void copy(
        TTypeLeft &         a_destination,
        const TTypeRight &  a_source,
        size_t              a_number_of_bytes
    )
    {
        assert( a_number_of_bytes > 0U);

        uint8_t * destination = reinterpret_cast< uint8_t*>( &a_destination);
        const uint8_t * source = reinterpret_cast< const uint8_t*>( &a_source);

        for ( size_t i = 0U; i < a_number_of_bytes; ++i)
        {
            destination[ i] = source[ i];
        }
    }

    // Return 'true' if both values are the same, 'false' otherwise.
    inline bool compare(
        uint8_t &       a_left,
        const uint8_t & a_right,
        size_t          a_number_of_bytes
    )
    {
        assert( a_number_of_bytes > 0U);

        for ( size_t i = 0U; i < a_number_of_bytes; ++i)
        {
            if ( ( &a_left)[ i] != ( &a_right)[ i])
            {
                return false;
            }
        }

        return true;
    }
}
