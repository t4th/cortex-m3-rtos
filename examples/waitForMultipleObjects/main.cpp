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

struct Data
{
    kernel::Handle events[ number_of_events];
};

// sets events in different timer intervals
void order_task( void * a_parameter)
{
    const char * number[ number_of_events] = 
    { "event 0", "event 1", "event 2", "event 3", "event 4", "event 5"};
    
    // Pass shared data through task parameter.
    Data & data = *reinterpret_cast< Data*>( a_parameter);

    print( "order_task: start.\n");

    while( true)
    {
        // Set all events.
        for ( uint32_t i = 0U; i < number_of_events; ++i)
        {
            print( "order_task: set ");
            print( number[ i]);
            print( ".\n");
            kernel::event::set( data.events[ i]);
            kernel::task::sleep( 100U);
        }
    };
}

// wait for all events to be set
void worker_task( void * a_parameter)
{
    // Pass shared data through task parameter.
    Data & data = *reinterpret_cast< Data*>( a_parameter);

    // Pass data through named system object.
    kernel::Handle manual_reset_event;
    
    bool handle_opened = kernel::event::open(
        manual_reset_event,
        "manual reset event"
    );

    if ( false == handle_opened)
    {
        print( "Failed to open named event.\n");
        kernel::hardware::debug::setBreakpoint();
    }

    print( "worker_task: start.\n");

    while ( true)
    {
        print( "worker_task: Start waiting for all events.\n");

        kernel::sync::WaitResult result = kernel::sync::waitForMultipleObjects(
            data.events,
            number_of_events,
            true,
            true
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
        kernel::event::reset( manual_reset_event);
    };
}

int main()
{
    Data data;

    kernel::init();

    // Create 5 automatic reset events.
    const uint32_t number_of_automatic_events = number_of_events - 1;

    for ( uint32_t i = 0U; i < number_of_automatic_events; ++i)
    {
        bool event_created = kernel::event::create( data.events[ i], false);

        if( false == event_created)
        {
            print( "Failed to create event\n");
            kernel::hardware::debug::setBreakpoint();
        }
    }

    // Create manual reset event
    bool event_created = kernel::event::create(
        data.events[ number_of_events - 1],
        true,
        "manual reset event"
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
