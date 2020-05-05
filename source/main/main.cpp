#include <kernel.hpp>
#include <task.hpp>
#include <scheduler.hpp>

void routine()
{
    volatile int a;
    while (1)
    {
        a++;
    }
}

int main()
{
    kernel::init();
    
    kernel::task::Id id;
    kernel::task::create(routine, kernel::task::Priority::Low, &id);
    kernel::scheduler::addTask(kernel::task::Priority::Low, id);

    kernel::start();
    
    for(;;);
}
