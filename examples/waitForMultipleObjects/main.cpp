// Test case: Test WaitForMultipleObjects function.
// Create Medium priority worker_task, and Low priority order_task.
// Worker_task is waiting for all events to be set to wake up.
// Order_task is setting events with a delay betwen each.
//
// To check all cases, 5 events are automatic reset, 1 event is manual
// reset.

#include <kernel.hpp>
#include "hardware/hardware.hpp"

void printText( const char * a_text)
{
    kernel::hardware::debug::print( a_text);
}

const uint32_t number_of_events = 6U;
const uint32_t manual_reset_event_index = 5U;

struct Data
{
    kernel::Handle events[ number_of_events];
};

// sets events in different timer intervals
void order_task( void * a_parameter)
{
    const char * number[ number_of_events] = 
    { "event 0", "event 1", "event 2", "event 3", "event 4", "event 5"};

    Data & data = *( ( Data*) a_parameter);

    printText( "order_task: start.\n");

    while( true)
    {
        // Set all events.
        for ( uint32_t i = 0U; i < number_of_events; ++i)
        {
            printText( "order_task: set ");
            printText( number[ i]);
            printText( ".\n");
            kernel::event::set( data.events[ i]);
            kernel::task::sleep( 100U);
        }
    };
}

// wait for all events to be set
void worker_task( void * a_parameter)
{
    Data & data = *( ( Data*) a_parameter);

    printText( "worker_task: start.\n");
    while ( true)
    {
        printText( "worker_task: Start waiting for all events.\n");

        kernel::sync::WaitResult result = kernel::sync::waitForMultipleObjects(
            data.events,
            number_of_events,
            true,
            true
        );

        if ( kernel::sync::WaitResult::ObjectSet == result)
        {
            printText( "worker_task: All objects are set.\n");
        }
        else
        {
            printText( "worker_task: Error occurred.\n");
        }

        printText( "worker_task: reset manual event.\n");
        kernel::event::reset( data.events[ manual_reset_event_index]);
    };
}

int main()
{
    Data data;

    kernel::init();

    // create automatic reset events
    const uint32_t number_of_automatic_event = number_of_events - 1U;

    for ( uint32_t i = 0U; i < number_of_automatic_event; ++i)
    {
        bool event_created = kernel::event::create( data.events[ i], false);

        if( false == event_created)
        {
            printText( "Failed to create event\n");
        }
    }

    // create manual reset event
    bool event_created = kernel::event::create( data.events[ manual_reset_event_index], true);

    if ( false == event_created)
    {
        printText( "Failed to create event\n");
    }

    kernel::task::create( order_task, kernel::task::Priority::Low, nullptr, &data);
    kernel::task::create( worker_task, kernel::task::Priority::Medium, nullptr, &data);

    kernel::start();

    for(;;);
}
