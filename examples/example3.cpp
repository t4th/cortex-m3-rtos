// Contains example used to verify if tasks are switching correctly.
// Tests:
// - creating software event
// - waitForSingleObject synchronization function

#include <kernel.hpp>
#include <hardware.hpp>

void printTask(const char * a_text)
{
    kernel::hardware::debug::print(a_text);
}

struct Events
{
    kernel::Handle event0;
};

void task0(void * a_parameter);
void task1(void * a_parameter);

int main()
{
    Events events;

    kernel::init();

    kernel::task::create(task0, kernel::task::Priority::Low, nullptr, &events);

    kernel::start();

    for(;;);
}

void task0(void * a_parameter)
{
    Events * events = (Events*)a_parameter;

    printTask("task 0 - start\r\n");

    kernel::Handle hTask1;

    if (kernel::task::create(
        task1,
        kernel::task::Priority::Medium,
        &hTask1,
        events,
        true
    ))
    {
        printTask("task 0 - created Medium suspended task 1\r\n");
    }

    if (kernel::event::create(events->event0))
    {
        printTask("task 0 - created event 0\r\n");
    }
    else
    {
        printTask("task 0 - create event failed\r\n");
    }

    printTask("task 0 - resuming task 1\r\n");
    kernel::task::resume(hTask1);

    while (true)
    {
        kernel::task::Sleep(500);
        printTask("task 0 - set event 0\r\n");
        kernel::event::set(events->event0);
    }
}

void task1(void * a_parameter)
{
    Events * events = (Events*)a_parameter;

    printTask("task 1 - start\r\n");

    while (true)
    {
        printTask("task 1 - wait forever for event 0\r\n");

        kernel::sync::WaitResult waitResult = kernel::sync::waitForSingleObject(events->event0);

        if (kernel::sync::WaitResult::ObjectSet == waitResult)
        {
            printTask("task 1 - wake up with object set\r\n");
        }
        else
        {
            printTask("task 1 - wake up failed\r\n");
        }

        kernel::task::Sleep(100);
    }
}