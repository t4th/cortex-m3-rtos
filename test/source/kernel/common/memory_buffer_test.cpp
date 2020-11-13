#include "catch.hpp"

#include "memory_buffer.hpp"

TEST_CASE("MemoryBuffer")
{
    kernel::common::MemoryBuffer<uint32_t, 3> buffer;
    uint32_t new_item_id = 0;

    SECTION ( "check if 'status' field is set correctly when adding new items")
    {
        REQUIRE(true == buffer.allocate(new_item_id));
        REQUIRE(0 == new_item_id);

        REQUIRE(true == buffer.allocate(new_item_id));
        REQUIRE(1 == new_item_id);

        REQUIRE(true == buffer.allocate(new_item_id));
        REQUIRE(2 == new_item_id);

        // try to overflow
        REQUIRE(false == buffer.allocate(new_item_id));
        
        // remove one item
        buffer.free(1);
        
        // allocate after using fere
        REQUIRE(true == buffer.allocate(new_item_id));
        
        // remove one item
        buffer.free(1);
        

        SECTION ( "check if data is set correctly with id returned from allocate function")
        {
            const uint32_t magic_number = 0xabc;
            REQUIRE(true == buffer.allocate(new_item_id));

            buffer.at(new_item_id) = magic_number;

            REQUIRE(magic_number== buffer.at(new_item_id));
        }
    }
}
