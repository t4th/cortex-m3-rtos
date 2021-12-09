// This is on target example, but you can still use Keil simulator and set
// Exti9_5 interrupt manually to pending state from within NVIC peripheral.

#include <kernel.hpp>

#include "gpio.hpp"

// TODO:
// - bind 4 buttons to Exti interrupts
// - use medium priority tasks to handle those exception,
//   while 4 low level tasks are blinking 4 LEDs.
// - use usart interrupt to send/receive data

extern "C"
{
    // On my board, button is connected to port PA8, which use EXTI line 8.
    void EXTI9_5_IRQHandler()
    {
        kernel::Handle sync_event;

        ( void)kernel::event::open( sync_event, "sync event");
        kernel::event::set( sync_event);

        // clear pending bit
        EXTI->PR |= EXTI_PR_PR8;
    }
}

// Setup hardware and resume worker task when done.
void startup_task( void * a_parameter)
{
    kernel::Handle & worker_task = *reinterpret_cast< kernel::Handle*>( a_parameter);

    // Create kernel object to sync actions
    kernel::Handle sync_event;

    ( void)kernel::event::create( sync_event, false, "sync event");

    // Clocks
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;

    // Configure external interrupt.
    EXTI->IMR |= EXTI_IMR_MR8;
    EXTI->FTSR |= EXTI_FTSR_TR8;

    // Configure and enable interrupt.
    constexpr uint32_t exti_9_5_interrupt_number = 23U;

    kernel::hardware::interrupt::priority::set(
        exti_9_5_interrupt_number,
        kernel::hardware::interrupt::priority::Preemption::User,
        kernel::hardware::interrupt::priority::Sub::Low
    );

    kernel::hardware::interrupt::enable( exti_9_5_interrupt_number);

    // gpio
    // PA8 is input pulled up to 3.3V
    gpio::setPinAsInput< gpio::Port::A, gpio::Pin::Number8, gpio::InputMode::Floating>();
    gpio::configureExternalInterrupt< gpio::Port::A, gpio::Pin::Number8>();

    kernel::task::resume( worker_task);
}

void worker_task( void * a_parameter)
{
    kernel::Handle sync_event;

    ( void)kernel::event::open( sync_event, "sync event");

    while( true)
    {
        // Wait 2 seconds for buttons press.
        auto result = kernel::sync::waitForSingleObject( sync_event, false, 2000U);

        if ( kernel::sync::WaitResult::ObjectSet == result)
        {
            kernel::hardware::debug::print( "Button pressed\n");
            // todo: add some form of debounce.
        }
        else
        {
            kernel::hardware::debug::print( "Wait timeout reached\n");
        }
    }
}

int main()
{
    kernel::Handle worker_task_handle;

    kernel::init();

    kernel::task::create(
        startup_task,
        kernel::task::Priority::Low,
        nullptr,            // No need to store startup task handle.
        &worker_task_handle // Pass worker handle as parameter.
    );

    // Store worker handle.
    kernel::task::create(
        worker_task,
        kernel::task::Priority::Low,
        &worker_task_handle,
        nullptr,    // No parameter.
        true        // Create suspended.
    );

    kernel::start();

    for(;;);
}