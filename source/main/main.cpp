#include <kernel.hpp>

void routine()
{
    volatile int i = 0;
    while (1)
    {

        for (; i < 10000; i++);
        break;
    }
}

int main()
{
    kernel::init();

    kernel::task::create(routine, kernel::task::Priority::Idle);

    kernel::start();
    
    for(;;);
}
