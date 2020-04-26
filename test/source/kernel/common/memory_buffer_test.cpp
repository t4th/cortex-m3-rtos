#include "catch.hpp"

#include "memory_buffer.hpp"

TEST_CASE("MemoryBuffer")
{
    kernel::common::MemoryBuffer<uint32_t, 3>::Context context;
    kernel::common::MemoryBuffer<uint32_t, 3> buffer(context);

    uint32_t new_item_id = 0;

    SECTION ( "check if 'status' field is set correctly when adding new items")
    {
        REQUIRE(true == buffer.allocate(new_item_id));
        REQUIRE(true == context.m_status[new_item_id]);

        REQUIRE(true == buffer.allocate(new_item_id));
        REQUIRE(true == context.m_status[new_item_id]);

        REQUIRE(true == buffer.allocate(new_item_id));
        REQUIRE(true == context.m_status[new_item_id]);

        SECTION ( "try to overflow")
        {
            REQUIRE(false == buffer.allocate(new_item_id));
        }

        SECTION ( "check if 'status' field is set correctly when removing items")
        {
            buffer.free(0);
            REQUIRE(false == context.m_status[0]);

            buffer.free(1);
            REQUIRE(false == context.m_status[1]);

            buffer.free(2);
            REQUIRE(false == context.m_status[2]);
        }
    }

    SECTION ( "check if data is set correctly with id returned from allocate function")
    {
        const uint32_t magic_number = 0xabc;
        REQUIRE(true == buffer.allocate(new_item_id));

        buffer.at(new_item_id) = magic_number;

        REQUIRE(true == context.m_status[new_item_id]);
        REQUIRE(magic_number== context.m_data[new_item_id]);
    }
}
