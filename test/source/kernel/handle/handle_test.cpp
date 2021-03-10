#include "catch.hpp"

#include "handle.hpp"

#include "task.hpp"
#include "event.hpp"
#include "timer.hpp"
#include "queue/queue.hpp"

TEST_CASE( "Handle")
{
    SECTION ( "Create handler and decapsulate it to different ID types.")
    {
        using namespace kernel::internal;
        
        uint32_t new_index = 0xdeadbeefU;

        kernel::Handle new_handle = handle::create( handle::ObjectType::Task, new_index);

        REQUIRE( reinterpret_cast< kernel::Handle>( 0x0000beefU) == new_handle);

        new_handle = handle::create( handle::ObjectType::Event, new_index);

        REQUIRE( reinterpret_cast< kernel::Handle>( 0x0002beefU) == new_handle);

        new_handle = handle::create( handle::ObjectType::Timer, new_index);

        REQUIRE( reinterpret_cast<kernel::Handle>( 0x0001beefU) == new_handle);

        new_handle = handle::create( static_cast< handle::ObjectType>( 0x1234U), new_index);

        REQUIRE( reinterpret_cast< kernel::Handle>( 0x1234beefU) == new_handle);
        REQUIRE( static_cast< handle::ObjectType>( 0x1234U) == handle::getObjectType( new_handle));

        // Create specyfic ID type from abstract index.
        {
            task::Id task = handle::getId< task::Id>( new_handle);
            event::Id event = handle::getId< event::Id>( new_handle);
            timer::Id timer = handle::getId< timer::Id>( new_handle);

            REQUIRE( 0xbeefU == task);
            REQUIRE( 0xbeefU == event);
            REQUIRE( 0xbeefU == timer);
        }
    }

    SECTION ( "Verify testCondition.")
    {
        using namespace kernel::internal;
        SECTION ( "Handle point to event.")
        {
            std::unique_ptr< timer::Context> timer_context( new timer::Context);
            std::unique_ptr< event::Context> event_context( new event::Context);
            std::unique_ptr< queue::Context> queue_context( new queue::Context);

            // Prepare event object and handle.
            event::Id new_index;
            bool event_created = event::create( *event_context, new_index, false);

            REQUIRE( true == event_created);

            // Create handle.
            kernel::Handle new_handle = handle::create( handle::ObjectType::Event, new_index);

            // Test the handle.
            // Expected: event is in Reset state, so check result should be false.
            bool condition_check_result = false;
            bool valid_handle = handle::testCondition(
                *timer_context,
                *event_context,
                *queue_context,
                new_handle,
                condition_check_result
            );

            REQUIRE( true == valid_handle);
            REQUIRE( false == condition_check_result);

            // Set event.
            event::set( *event_context, new_index);

            // Test the handle.
            // Expected: event is in SET state, so check result should be true.
            valid_handle = handle::testCondition(
                *timer_context,
                *event_context,
                *queue_context,
                new_handle,
                condition_check_result
            );

            REQUIRE( true == valid_handle);
            REQUIRE( true == condition_check_result);
        }

        SECTION ( "Handle point to timer.")
        {
            std::unique_ptr< timer::Context> timer_context( new timer::Context);
            std::unique_ptr< event::Context> event_context( new event::Context);
            std::unique_ptr< queue::Context> queue_context( new queue::Context);

            // Prepare event object and handle.
            timer::Id new_index;
            kernel::Time_ms start = 0U;
            kernel::Time_ms interval = 100U;
            bool timer_created = timer::create( *timer_context, new_index, start, interval);

            REQUIRE( true == timer_created);

            // Create handle.
            kernel::Handle new_handle = handle::create( handle::ObjectType::Timer, new_index);

            // Test the handle.
            // Expected: Timer is in Started state and condition should be false.
            bool condition_check_result = false;
            bool valid_handle = handle::testCondition(
                *timer_context,
                *event_context,
                *queue_context,
                new_handle,
                condition_check_result
            );

            REQUIRE( true == valid_handle);
            REQUIRE( false == condition_check_result);

            // Finish timer.
            kernel::Time_ms current_time = start + interval + 1U;
            timer::tick( *timer_context, current_time);

            // Test the handle.
            // Expected: Timer is in Finished state and condition should be true.
            valid_handle = handle::testCondition(
                *timer_context,
                *event_context,
                *queue_context,
                new_handle,
                condition_check_result
            );

            REQUIRE( true == valid_handle);
            REQUIRE( true == condition_check_result);
        }

        SECTION ( "Handle point to unsupported system object.")
        {
            std::unique_ptr< timer::Context> timer_context( new timer::Context);
            std::unique_ptr< event::Context> event_context( new event::Context);
            std::unique_ptr< queue::Context> queue_context( new queue::Context);

            kernel::Handle invalid_handle = ( kernel::Handle) 0xaf23123U;

            // Invalid handle should result in funtion returning false.
            bool condition_check_result = false;
            bool valid_handle = handle::testCondition(
                *timer_context,
                *event_context,
                *queue_context,
                invalid_handle,
                condition_check_result
            );

            REQUIRE( false == valid_handle);
            REQUIRE( false == condition_check_result);
        }
    }
}