// Contains example used to verify if tasks are switching correctly.
// Tests
// - Sleep function

#include <kernel.hpp>

struct Delay
{
    kernel::TimeMs delay_for_task0;
    kernel::TimeMs delay_for_task1;
    kernel::TimeMs delay_for_task2;
};

void task_routine( void * a_parameter);

int main()
{
    Delay delay;

    delay.delay_for_task0 = 100U;
    delay.delay_for_task1 = 500U;
    delay.delay_for_task2 = 1000U;

    kernel::init();

    kernel::task::create( task_routine, kernel::task::Priority::Low, nullptr, &delay.delay_for_task0);
    kernel::task::create( task_routine, kernel::task::Priority::Low, nullptr, &delay.delay_for_task1);
    kernel::task::create( task_routine, kernel::task::Priority::Low, nullptr, &delay.delay_for_task2);

    kernel::start();

    for(;;);
}

// Delayed ping.
void task_routine( void * a_parameter)
{
    kernel::TimeMs & timer = *reinterpret_cast< kernel::TimeMs*>( a_parameter);

    kernel::hardware::debug::print( "task - start\r\n");

    while ( true)
    {
        kernel::task::sleep( timer);

        switch( timer)
        {
        case 100U:
            kernel::hardware::debug::print( "task 0 - ping\r\n");
            break;
        case 500U:
            kernel::hardware::debug::print( "task 1 - ping\r\n");
            break;
        case 1000U:
            kernel::hardware::debug::print( "task 2 - ping\r\n");
            break;
        }
    }
}
