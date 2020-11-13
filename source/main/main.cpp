#include <kernel.hpp>
#include <hardware.hpp>

void printTask(const char * text)
{
    kernel::hardware::debug::print(text);
}

void task0(void * a_parameter);
void task1(void * a_parameter);
void task2(void * a_parameter);
void task3(void * a_parameter);

struct
{
    kernel::task::Id task0;
    kernel::task::Id task3;
} ids;

void cleanupTask(void * a_parameter)
{
    volatile int i = 0;
    for (i = 0; i < 3000000; i++);
    
    printTask("terminate task 0 and 3\r\n");
    kernel::task::terminate(ids.task0);
    kernel::task::terminate(ids.task3);
    printTask("cleanup task - end\r\n");
}

void task0(void * a_parameter)
{
    volatile int i = 0;
    printTask("task 0 - start\r\n");
    while (1)
    {
        for (i = 0; i < 100000; i++);
        printTask("task 0 - ping\r\n");
    }
}

void task1(void * a_parameter)
{
    volatile int i = 0;
    static bool once = true;

    printTask("task 1 - start\r\n");
    while (1)
    {
        if (once)
        {
            for (i = 0; i < 1000000; i++);
            once = false;
            printTask("task 1 - created new task 2 Medium\r\n");
            if (false == kernel::task::create(task2, kernel::task::Priority::Medium))
            {
                printTask("task 1 failed to create task 2\r\n");
            }
            printTask("task 1 - end\r\n");
            break;
        }
        else
        {
            static int die = 0;
            die ++;
            if ( die >= 20)
            {
                printTask("task 1 - end\r\n");
                break;
            }

            for (i = 0; i < 100000; i++);
            printTask("task 1 - ping\r\n");
        }
    }
}

void task2(void * a_parameter)
{
    volatile int i = 0;
    printTask("task 2 - start\r\n");
    while (1)
    {
        for (i = 0; i < 1200000; i++);
        printTask("task 2 - created new task 1 Low\r\n");
        if (false == kernel::task::create(task1, kernel::task::Priority::Low))
        {
            printTask("task 2 failed to create task 1\r\n");
        }
        printTask("task 2 - end\r\n");
        break;
    }
}

void task3(void * a_parameter)
{
    volatile int i = 0;
    printTask("task 3 - start\r\n");
    while (1)
    {

        for (i = 0; i < 100000; i++);
        printTask("task 3 - ping\r\n");
    }
}

int main()
{
    kernel::init();
    
    kernel::task::create(task0, kernel::task::Priority::Low, &ids.task0);
    kernel::task::create(task1, kernel::task::Priority::Low);
    kernel::task::create(task3, kernel::task::Priority::Low, &ids.task3);
    kernel::task::create(cleanupTask, kernel::task::Priority::Low);


    kernel::start();
    
    for(;;);
}
