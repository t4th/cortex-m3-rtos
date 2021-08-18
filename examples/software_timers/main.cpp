// Contains example used to verify if software timers are working.
// This is based on task_sleep example.
// Tests
// - Software Timers

#include <kernel.hpp>

enum class Task
{
    Number0,
    Number1,
    Number2
};
// Wait forever for passed Handle parameter to be in SET condition.
template < Task task>
void task_routine( void * a_parameter)
{
    kernel::Handle & timer = *reinterpret_cast< kernel::Handle*>( a_parameter);

    kernel::hardware::debug::print( "task - start\r\n");

    // Start software timer.
    kernel::timer::start( timer);

    while ( true)
    {
        // Wait forever for timer to be in Finished state.
        // Signal that woke the task will be reset. In this case Timer will be Stopped.
        if ( kernel::sync::WaitResult::ObjectSet == kernel::sync::waitForSingleObject( timer, true))
        {
            const char * task_name = nullptr;
            
            switch ( task)
            {
            case Task::Number0: task_name = "Task 0"; break;
            case Task::Number1: task_name = "Task 1"; break;
            case Task::Number2: task_name = "Task 2"; break;
            }

            kernel::hardware::debug::print( task_name);
            kernel::hardware::debug::print( " - ping\r\n");
            
            // Restart software timer.
            kernel::timer::restart( timer);

        }
        else
        {
            kernel::hardware::debug::print( "error\r\n");
        }
    }
}

int main()
{
    kernel::Handle timer0;
    kernel::Handle timer1;
    kernel::Handle timer2;

    ( void)kernel::timer::create( timer0, 100U);
    ( void)kernel::timer::create( timer1, 500U);
    ( void)kernel::timer::create( timer2, 1000U);

    kernel::init();

    kernel::task::create( task_routine< Task::Number0>, kernel::task::Priority::Low, nullptr, &timer0);
    kernel::task::create( task_routine< Task::Number1>, kernel::task::Priority::Low, nullptr, &timer1);
    kernel::task::create( task_routine< Task::Number2>, kernel::task::Priority::Low, nullptr, &timer2);

    kernel::start();

    for(;;);
}
