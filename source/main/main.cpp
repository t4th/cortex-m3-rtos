#include <kernel.hpp>
#include <hardware.hpp>

void printTask(const char * text, volatile int asd)
{
    kernel::hardware::debug::print(text);
}

void task0()
{
    volatile int i = 0;
    volatile int b = 0x123;
    while (1)
    {
        printTask("task 0\r\n", b);
        for (i = 0; i < 100000; i++);
    }
}

void task1()
{
    volatile int i = 0;
    volatile int b = 0x456;
    while (1)
    {
        printTask("task 1\r\n",b);
        for (i = 0; i < 1000000; i++);
    }
}

void task2()
{
    volatile int i = 0;
    volatile int b = 0x887;
    while (1)
    {
        printTask("task 2\r\n",b);
        for (i = 0; i < 100000; i++);
    }
}

void task3()
{
    volatile int i = 0;
    while (1)
    {

        for (i = 0; i < 600000; i++);
        printTask("task 3 end\r\n", 0xdeadbeef);
        break;
    }
}

int main()
{
    kernel::init();
    
    //kernel::task::create(routine, kernel::task::Priority::Idle);
    kernel::task::create(task0, kernel::task::Priority::Low);
    kernel::task::create(task1, kernel::task::Priority::Low);
    kernel::task::create(task2, kernel::task::Priority::Low);
    kernel::task::create(task3, kernel::task::Priority::Low);


    kernel::start();
    
    for(;;);
}
