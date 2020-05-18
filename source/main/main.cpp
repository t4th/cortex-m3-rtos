#include <kernel.hpp>

void routine()
{
    while (1)
    {
    }
}

int main()
{
    kernel::init();

    kernel::task::create(routine, kernel::task::Priority::Idle);

    kernel::start();
    
    for(;;);
}
