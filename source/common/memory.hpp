#pragma once

#include <cstdint>
#include <cassert>

// Note: I just didn't want to include whole <cstring>. 
//       Also references are so much nicer.
namespace kernel::internal::memory
{
    inline void copy(
        uint8_t &       a_destination,
        const uint8_t & a_source,
        size_t          a_number_of_bytes
    )
    {
        assert( a_number_of_bytes > 0U);

        for ( size_t i = 0U; i < a_number_of_bytes; ++i)
        {
            ( &a_destination)[ i] = ( &a_source)[ i];
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
