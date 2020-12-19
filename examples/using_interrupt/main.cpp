
#include <kernel.hpp>
#include <hardware.hpp>

void printTask( const char * a_text)
{
    kernel::hardware::debug::print( a_text);
}

// todo
// - bind 4 buttons to Exti interrupts
// - use medium priority tasks to handle those exception,
//   while 4 low level tasks are blinking 4 LEDs.
// - use usart interrupt to send/receive data

int main()
{

    kernel::hardware::critical_section::Context context;
    kernel::hardware::critical_section::lock(context);

    kernel::init();

    kernel::hardware::critical_section::unlock(context);

    kernel::start();

    for(;;);
}