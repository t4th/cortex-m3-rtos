// This is on target example, because it is using MCU vendor defined interrupt.

#include <kernel.hpp>

#include <hardware.hpp>

#include "gpio.hpp"


// todo
// - bind 4 buttons to Exti interrupts
// - use medium priority tasks to handle those exception,
//   while 4 low level tasks are blinking 4 LEDs.
// - use usart interrupt to send/receive data

struct Shared
{
    kernel::Handle m_event;
    kernel::Handle m_worker_task;
};

// TODO: how to pass data to interrupt handler without globals?
//       named events?
Shared shared_data;

void print( const char * a_text)
{
    kernel::hardware::debug::print( a_text);
}

extern "C"
{
    // On my board, button is connected to port PA8, which use EXTI line 8.
    void EXTI9_5_IRQHandler()
    {
        kernel::hardware::critical_section::Context context;
        kernel::hardware::critical_section::lock( context);
        
        // todo: create setFromInterrupt function
        kernel::event::set( shared_data.m_event);

        kernel::hardware::critical_section::unlock( context);

        // clear pending bit
        EXTI->PR |= EXTI_PR_PR8;
    }
}

// Setup hardware and resume worker task when done.
void startup_task( void * a_parameter)
{
    Shared & shared_data = *( ( Shared*) a_parameter);

    // create kernel object to sync actions
    kernel::event::create( shared_data.m_event);

    // clocks
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;

    // exti
    EXTI->IMR |= EXTI_IMR_MR8;
    EXTI->FTSR |= EXTI_FTSR_TR8;

    // interrupts
    NVIC_EnableIRQ( EXTI9_5_IRQn);

    // gpio
    // PA8 is input pulled up to 3.3V
    gpio::setPinAsInput< gpio::Port::A, gpio::Pin::Number8, gpio::InputMode::Floating>();
    gpio::configureExternalInterrupt< gpio::Port::A, gpio::Pin::Number8>();

    kernel::task::resume( shared_data.m_worker_task);
}

void worker_task( void * a_parameter)
{
    Shared & shared_data = *( (Shared*) a_parameter);

    while( true)
    {
        // Wait 2 seconds for buttons press.
        auto result = kernel::sync::waitForSingleObject( shared_data.m_event, false, 2000U);

        if ( kernel::sync::WaitResult::ObjectSet == result)
        {
            kernel::hardware::debug::print( "Button pressed\n");
            // todo: add some form of debounce.
        }
        else
        {
            kernel::hardware::debug::print( "Wait failed\n");
        }
    }
}

int main()
{
    kernel::init();

    kernel::task::create( startup_task, kernel::task::Priority::Low, nullptr, &shared_data);
    kernel::task::create( worker_task, kernel::task::Priority::Low, &shared_data.m_worker_task, &shared_data, true);

    kernel::start();

    for(;;);
}