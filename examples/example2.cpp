// Contains example used to verify if tasks are switching correctly.
// Tests
// - Sleep function

#include <kernel.hpp>
#include <hardware.hpp>

struct timer
{
    kernel::Time_ms time0;
    kernel::Time_ms time1;
    kernel::Time_ms time2;
};

void printTask(const char * a_text)
{
    kernel::hardware::debug::print(a_text);
}

void task0(void * a_parameter);

int main()
{
    timer timer;
    timer.time0 = 100;
    timer.time1 = 500;
    timer.time2 = 1000;

    kernel::init();

    kernel::task::create(task0, kernel::task::Priority::Low, nullptr, &timer.time0);
    kernel::task::create(task0, kernel::task::Priority::Low, nullptr, &timer.time1);
    kernel::task::create(task0, kernel::task::Priority::Low, nullptr, &timer.time2);

    kernel::start();

    for(;;);
}

// Delayed ping.
void task0(void * a_parameter)
{
    kernel::Time_ms * timer = (kernel::Time_ms*)a_parameter;
    printTask("task - start\r\n");

    while (true)
    {
        kernel::task::Sleep(*timer);
        switch(*timer)
        {
        case 100:
            printTask("task 0 - ping\r\n");
            break;
        case 500:
            printTask("task 1 - ping\r\n");
            break;
        case 1000:
            printTask("task 2 - ping\r\n");
            break;
        }
    }
}
