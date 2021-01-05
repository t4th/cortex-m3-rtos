// Contains example used to verify if tasks are switching correctly.
// Tests
// - Sleep function

#include <kernel.hpp>
#include "hardware/hardware.hpp"

struct timer
{
    kernel::Time_ms time0;
    kernel::Time_ms time1;
    kernel::Time_ms time2;
};

void printText( const char * a_text)
{
    kernel::hardware::debug::print( a_text);
}

void task0( void * a_parameter);

int main()
{
    timer timer;
    timer.time0 = 100U;
    timer.time1 = 500U;
    timer.time2 = 1000U;

    kernel::init();

    kernel::task::create( task0, kernel::task::Priority::Low, nullptr, &timer.time0);
    kernel::task::create( task0, kernel::task::Priority::Low, nullptr, &timer.time1);
    kernel::task::create( task0, kernel::task::Priority::Low, nullptr, &timer.time2);

    kernel::start();

    for(;;);
}

// Delayed ping.
void task0( void * a_parameter)
{
    kernel::Time_ms * timer = ( kernel::Time_ms*) a_parameter;
    printText( "task - start\r\n");

    while ( true)
    {
        kernel::task::sleep( *timer);
        switch( *timer)
        {
        case 100U:
            printText( "task 0 - ping\r\n");
            break;
        case 500U:
            printText( "task 1 - ping\r\n");
            break;
        case 1000U:
            printText( "task 2 - ping\r\n");
            break;
        }
    }
}
