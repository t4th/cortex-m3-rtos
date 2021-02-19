// This is on target example, but you can still use Keil simulator and set
// USART interrupts manually to pending state from within NVIC peripheral.

// Example: Echo input string from usart 1 to ITM trace builit-in in debugger.

#include <kernel.hpp>

#include "gpio.hpp"


// todo
// - init usart
// - implement rx queue and interrupt
// - implement tx queue and interrupt

struct Shared
{
    static constexpr size_t max_queue_elements{ 64U};

    kernel::Handle m_usart_byte_rx;
    kernel::Handle m_usart_byte_tx;
    kernel::containers::StaticQueue< uint8_t, max_queue_elements> m_usart_rx_queue;
    kernel::containers::StaticQueue< uint8_t, max_queue_elements> m_usart_tx_queue;

    kernel::Handle m_worker_task;
};

// TODO: how to pass data to interrupt handler without globals?
//       named events?
Shared g_shared_data;

void print( const char * a_text)
{
    kernel::hardware::debug::print( a_text);
}

extern "C"
{
    void USART1_IRQHandler()
    {
       volatile uint16_t status_register = USART1->SR;

        if ( status_register & USART_SR_RXNE)
        {
            uint8_t received_byte = USART1->DR & 0xFF;
            
            using namespace kernel::hardware;

            critical_section::Context cs;
            critical_section::enter( cs, interrupt::priority::Preemption::User);
            {
                bool queue_not_full =
                    g_shared_data.m_usart_rx_queue.push( received_byte);

                if ( false == queue_not_full)
                {
                        // todo: make overflow event
                        debug::print( "Rx queue push overflow\n");
                }
            }
            critical_section::leave( cs);

            kernel::event::setFromInterrupt( g_shared_data.m_usart_byte_rx);

            // todo: check if clearing pending bit is not reordered
            USART1->SR &= ~USART_SR_RXNE;
        }
    
        if ( status_register & USART_SR_TXE)
        {
            USART1->CR1 &= ~USART_CR1_TXEIE;
        }
    }
}

// Setup hardware and resume worker task when done.
void startup_task( void * a_parameter)
{
    Shared & shared_data = *( ( Shared*) a_parameter);

    // create kernel synchronization events
    kernel::event::create( shared_data.m_usart_byte_rx);
    kernel::event::create( shared_data.m_usart_byte_tx);

    // clocks
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

    // configure gpio
    // On my board, USART1 is connected to GPIOA pin 9 (tx) and 10 (rx).
    gpio::setPinAsOutput<
        gpio::Port::A,
        gpio::Pin::Number9,
        gpio::OutputSpeed::Max50MHZ,
        gpio::OutputMode::AlternateFunctionPushPull>();

    gpio::setPinAsInput< gpio::Port::A, gpio::Pin::Number10, gpio::InputMode::Floating>();

    // configure usart 1
    const uint32_t  core_frequency = kernel::getCoreFrequencyHz();
    const uint32_t  usart_baudrate_9600{ core_frequency / 9600U};

    USART1->BRR = static_cast< uint16_t>( usart_baudrate_9600);
    USART1->CR1 |=  USART_CR1_RXNEIE  // enable rx ready interrupt
                |   USART_CR1_RE;      // enable rx
                //|   USART_CR1_TE;     // enable tx

    // interrupts
    constexpr uint32_t usart1_interrupt_number = 37U;

    kernel::hardware::interrupt::priority::set(
        usart1_interrupt_number,
        kernel::hardware::interrupt::priority::Preemption::User,
        kernel::hardware::interrupt::priority::Sub::Low
    );

    kernel::hardware::interrupt::enable( usart1_interrupt_number);

    // start usart 1
    USART1->CR1 |= USART_CR1_UE;

    kernel::task::resume( shared_data.m_worker_task);
}

void worker_task( void * a_parameter)
{
    Shared & shared_data = *( ( Shared*) a_parameter);

    while( true)
    {
        auto result = kernel::sync::waitForSingleObject( shared_data.m_usart_byte_rx);

        if ( kernel::sync::WaitResult::ObjectSet == result)
        {
            uint8_t received_byte;
            bool print_word = false;
            size_t number_of_elements = 0U;

            using namespace kernel::hardware;

            critical_section::Context cs;

            // Flush all queue content until its empty.
            while ( true)
            {
                bool queue_not_empty = false;

                critical_section::enter( cs, interrupt::priority::Preemption::User);
                {
                    queue_not_empty = g_shared_data.m_usart_rx_queue.pop( received_byte);
                }
                critical_section::leave( cs);

                if ( true == queue_not_empty)
                {
                    debug::putChar( static_cast< char>( received_byte));
                }
                else
                {
                    // queue is empty
                    break;
                }
            };
        }
        else
        {
            kernel::hardware::debug::print( "\nError.\n");
        }
    }
}

int main()
{
    kernel::init();

    kernel::task::create( startup_task, kernel::task::Priority::Low, nullptr, &g_shared_data);
    kernel::task::create( worker_task, kernel::task::Priority::Low, &g_shared_data.m_worker_task, &g_shared_data, true);

    kernel::start();

    for(;;);
}