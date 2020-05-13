#include <kernel.hpp>

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

    kernel::task::create(routine, kernel::task::Priority::Idle);

    kernel::start();
    
    for(;;);
}
