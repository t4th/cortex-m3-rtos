// Contains example used to verify if tasks are switching correctly.

#include <kernel.hpp>
#include <hardware.hpp>

struct Task_ids
{
    kernel::Handle task0;
    kernel::Handle task2;
    kernel::Handle task3;
};

void delay(uint32_t a_ticks)
{
    for (uint32_t i = 0U; i < a_ticks; i++);
}

void printTask(const char * a_text)
{
    kernel::hardware::debug::print(a_text);
}

void task0(void * a_parameter);
void task1(void * a_parameter);
void task2(void * a_parameter);
void task3(void * a_parameter);
void cleanupTask(void * a_parameter);

int main()
{
    Task_ids task_ids;

    kernel::init();

    kernel::task::create(task0, kernel::task::Priority::Low, &task_ids.task0);
    kernel::task::create(task1, kernel::task::Priority::Low, nullptr, &task_ids);
    kernel::task::create(task3, kernel::task::Priority::Low, &task_ids.task3);
    kernel::task::create(cleanupTask, kernel::task::Priority::Low, nullptr, &task_ids);

    kernel::start();

    for(;;);
}

// Terminate task0 and task3 after some delay and exit.
void cleanupTask(void * a_parameter)
{
    Task_ids * ids = (Task_ids*)a_parameter;

    delay(2000000U);

    printTask("terminate task 0 and 3\r\n");
    kernel::task::terminate(ids->task0);
    kernel::task::terminate(ids->task3);
    kernel::task::resume(ids->task2);
    printTask("cleanup task - end\r\n");
}

// Delayed ping.
void task0(void * a_parameter)
{
    printTask("task 0 - start\r\n");

    while (true)
    {
        delay(100000U);
        printTask("task 0 - ping\r\n");
    }
}

// After first creation, create Medium task2 and exit.
// On second creation ping for some time until killing itself.
void task1(void * a_parameter)
{
    Task_ids * ids = (Task_ids*)a_parameter;
    int die = 0;
    static bool once = true;

    printTask("task 1 - start\r\n");

    while (true)
    {
        if (once)
        {
            once = false;

            delay(1000000U);

            printTask("task 1 - created new task 2 Medium\r\n");

            if (false == kernel::task::create(task2, kernel::task::Priority::Medium, &(ids->task2), a_parameter))
            {
                printTask("task 1 failed to create task 2\r\n");
            }

            printTask("task 1 - end\r\n");

            // Terminate itself.
            kernel::Handle handle = kernel::task::getCurrent();
            kernel::task::terminate(handle);
        }
        else
        {
            if ( ++die >= 20)
            {
                printTask("task 1 - end\r\n");
                break;
            }

            delay(100000U);

            printTask("task 1 - ping\r\n");
        }
    }
}

// Medium priority task blocks all Low tasks.
void task2(void * a_parameter)
{
    printTask("task 2 - start and block LOW\r\n");

    while (true)
    {
        delay(1200000U);

        printTask("task 2 - created new task 1 Low\r\n");

        if (false == kernel::task::create(task1, kernel::task::Priority::Low, nullptr, a_parameter))
        {
            printTask("task 2 failed to create task 1\r\n");
        }
        printTask("task 2 - end and release LOW\r\n");

        auto handle = kernel::task::getCurrent();
        kernel::task::suspend(handle);
    }
}

// Delayed ping.
void task3(void * a_parameter)
{
    printTask("task 3 - start\r\n");

    while (true)
    {
        delay(100000U);
        printTask("task 3 - ping\r\n");
    }
}
