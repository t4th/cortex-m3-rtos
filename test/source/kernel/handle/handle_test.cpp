#include "catch.hpp"

#include "handle.hpp"

#include "task.hpp"
#include "event.hpp"
#include "timer.hpp"

TEST_CASE("Handle")
{
    SECTION ("Create handler and decapsulate it to different ID types.")
    {
        using namespace kernel::internal;
        
        kernel::Handle new_handle;
        uint32_t new_index = 0xdeadbeef;

        new_handle = handle::create(handle::ObjectType::Task, new_index);

        REQUIRE(reinterpret_cast<kernel::Handle>(0x0000beefU) == new_handle);

        new_handle = handle::create(handle::ObjectType::Event, new_index);

        REQUIRE(reinterpret_cast<kernel::Handle>(0x0002beefU) == new_handle);

        new_handle = handle::create(handle::ObjectType::Timer, new_index);

        REQUIRE(reinterpret_cast<kernel::Handle>(0x0001beefU) == new_handle);

        new_handle = handle::create(static_cast<handle::ObjectType>(0x1234U), new_index);

        REQUIRE(reinterpret_cast<kernel::Handle>(0x1234beefU) == new_handle);
        REQUIRE(static_cast<handle::ObjectType>(0x1234U) == handle::getObjectType(new_handle));

        // Create specyfic ID type from abstract index.
        {
            task::Id task = handle::getId<task::Id>(new_handle);
            event::Id event = handle::getId<event::Id>(new_handle);
            timer::Id timer = handle::getId<timer::Id>(new_handle);

            REQUIRE(0xbeefU == task);
            REQUIRE(0xbeefU == event);
            REQUIRE(0xbeefU == timer);
        }
    }
}