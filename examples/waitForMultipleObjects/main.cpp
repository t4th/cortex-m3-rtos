// Test case: Test WaitForMultipleObjects function.
// Create Medium priority worker_task, and Low priority order_task.
// Worker_task is waiting for all events to be set to wake up.
// Order_task is setting events with a delay betwen each.
//
// To check all cases, 5 events are automatic reset, 1 event is manual
// reset.

#include <kernel.hpp>

using kernel::hardware::debug::print;
using kernel::hardware::debug::setBreakpoint;

static constexpr uint32_t number_of_events = 6U;

// Define shared data between tasks.
// Contains array with handles to all waitable events.
// Last event in an array is manual reset event.
struct Data
{
    kernel::Handle events[ number_of_events];
};

// Sets events every 100ms.
void order_task( void * a_parameter)
{
    const char * event_name[ number_of_events] = 
    { "event 0", "event 1", "event 2", "event 3", "event 4", "event 5 (manual reset)"};
    
    // Pass shared data through task parameter.
    Data & data = *reinterpret_cast< Data*>( a_parameter);

    print( "order_task: start.\n");

    while( true)
    {
        // Set all events.
        for ( uint32_t i = 0U; i < number_of_events; ++i)
        {
            print( "order_task: set ");
            print( event_name[ i]);
            print( ".\n");
            kernel::event::set( data.events[ i]);
            kernel::task::sleep( 100U);
        }
    };
}

// Wait for all events to be set
void worker_task( void * a_parameter)
{
    // Pass shared data through task parameter.
    Data & data = *reinterpret_cast< Data*>( a_parameter);

    print( "worker_task: start.\n");

    while ( true)
    {
        print( "worker_task: Start waiting for all events.\n");

        kernel::sync::WaitResult result = kernel::sync::waitForMultipleObjects(
            data.events,
            number_of_events,
            true, // Wait for all events to be set.
            true  // Wait forever.
        );

        if ( kernel::sync::WaitResult::ObjectSet == result)
        {
            print( "worker_task: All objects are set.\n");
        }
        else
        {
            print( "worker_task: Error occurred.\n");
            kernel::hardware::debug::setBreakpoint();
        }

        print( "worker_task: reset manual event.\n");
        constexpr uint32_t manual_event_index { number_of_events - 1U};

        kernel::event::reset( data.events[ manual_event_index]);
    };
}

int main()
{
    Data data{};

    kernel::init();

    // Create 5 automatic reset events. Leave 6th spot for manual reset event.
    const uint32_t number_of_automatic_events = number_of_events - 1U;

    for ( uint32_t i = 0U; i < number_of_automatic_events; ++i)
    {
        bool event_created = kernel::event::create( data.events[ i], false);

        if( false == event_created)
        {
            print( "Failed to create event\n");
            kernel::hardware::debug::setBreakpoint();
        }
    }

    // Create manual reset event.
    constexpr uint32_t manual_event_index = number_of_events - 1U;

    bool event_created = kernel::event::create(
        data.events[ manual_event_index],
        true
    );

    if ( false == event_created)
    {
        print( "Failed to create event\n");
        kernel::hardware::debug::setBreakpoint();
    }

    kernel::task::create( order_task, kernel::task::Priority::Low, nullptr, &data);
    kernel::task::create( worker_task, kernel::task::Priority::Medium, nullptr, &data);

    kernel::start();

    for(;;);
}
