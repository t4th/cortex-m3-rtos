#include <kernel.hpp>
#include <hardware.hpp>

void task0()
{
    volatile int i = 0;
    while (1)
    {
        kernel::hardware::debug::print("task 0\r\n");
        for (i = 0; i < 100000; i++);
    }
}

void task1()
{
    volatile int i = 0;
    while (1)
    {
        kernel::hardware::debug::print("task 1\r\n");
        for (i = 0; i < 100000; i++);
    }
}

void task2()
{
    volatile int i = 0;
    while (1)
    {
        kernel::hardware::debug::print("task 2\r\n");
        for (i = 0; i < 100000; i++);
    }
}

void task3()
{
    volatile int i = 0;
    while (1)
    {

        for (i = 0; i < 600000; i++);
        kernel::hardware::debug::print("task 3 end\r\n");
        break;
    }
}

int main()
{
    //kernel::task::create(routine, kernel::task::Priority::Idle);
    kernel::task::create(task0, kernel::task::Priority::Idle);
    kernel::task::create(task1, kernel::task::Priority::Idle);
    kernel::task::create(task2, kernel::task::Priority::Idle);
    kernel::task::create(task3, kernel::task::Priority::Idle);
    kernel::init();


    kernel::start();
    
    for(;;);
}
