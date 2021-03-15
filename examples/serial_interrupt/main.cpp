// This is on target example, but you can still use Keil simulator and set
// USART interrupts manually to pending state from within NVIC peripheral.

// Example: Echo input string from usart 1 to ITM trace builit-in in debugger.

#include <kernel.hpp>

#include "gpio.hpp"


// TODO
// - init usart
// - implement rx queue and interrupt
// - implement tx queue and interrupt

struct Shared
{
    kernel::Handle m_usart_rx_queue;
    kernel::Handle m_usart_tx_queue;

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
            
            bool queue_not_full = kernel::static_queue::send(
                    g_shared_data.m_usart_rx_queue,
                    received_byte
                );

            if ( false == queue_not_full)
            {
                // TODO: make overflow event
                kernel::hardware::debug::print( "Rx queue push overflow\n");
            }

            // TODO: check if clearing pending bit is not reordered
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
    Shared & shared_data = *reinterpret_cast< Shared*>( a_parameter);

    // Enable clocks for GPIOA and USART1.
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

    // Configure GPIOA pins used by USART1 peripheral.
    // NOTE: On my board, USART1 is connected to GPIOA pin 9 (tx) and 10 (rx).
    gpio::setPinAsOutput<
        gpio::Port::A,
        gpio::Pin::Number9,
        gpio::OutputSpeed::Max50MHZ,
        gpio::OutputMode::AlternateFunctionPushPull>();

    gpio::setPinAsInput<
        gpio::Port::A,
        gpio::Pin::Number10,
        gpio::InputMode::Floating>();

    // Configure USART1: baudrate and interrupts.
    const uint32_t  core_frequency = kernel::getCoreFrequencyHz();
    const uint32_t  usart_baudrate_9600{ core_frequency / 9600U};

    USART1->BRR = static_cast< uint16_t>( usart_baudrate_9600);
    USART1->CR1 |=  USART_CR1_RXNEIE   // Enable rx ready interrupt.
                |   USART_CR1_RE;      // Enable rx.
                //|   USART_CR1_TE;    // Enable tx.

    // Enable USART1 interrupt in Interrupt Controller (NVIC).
    constexpr uint32_t usart1_interrupt_number = 37U;

    kernel::hardware::interrupt::priority::set(
        usart1_interrupt_number,
        kernel::hardware::interrupt::priority::Preemption::User,
        kernel::hardware::interrupt::priority::Sub::Low
    );

    kernel::hardware::interrupt::enable( usart1_interrupt_number);

    // Start USART1.
    USART1->CR1 |= USART_CR1_UE;

    kernel::task::resume( shared_data.m_worker_task);
}

void worker_task( void * a_parameter)
{
    Shared & shared_data = *( ( Shared*) a_parameter);

    while( true)
    {
        // Wait until at least one elements is available in queue.
        auto result = kernel::sync::waitForSingleObject( shared_data.m_usart_rx_queue);

        if ( kernel::sync::WaitResult::ObjectSet == result)
        {
            uint8_t received_byte;

            // Flush all queue content until its empty.
            while ( true)
            {
                bool queue_not_empty = kernel::static_queue::receive(
                        g_shared_data.m_usart_rx_queue,
                        received_byte
                    );

                if ( true == queue_not_empty)
                {
                    kernel::hardware::debug::putChar( static_cast< char>( received_byte));
                }
                else
                {
                    // Queue is empty.
                    break;
                }
            };
        }
        else
        {
            kernel::hardware::debug::print( "\nWaitForSingleObject error.\n");
        }
    }
}

int main()
{
    static constexpr size_t max_queue_elements{ 32U};

    // TODO: Consider buffer to be volatile, since its used in interrupt.
    kernel::static_queue::Buffer< uint8_t, max_queue_elements> usart_rx_buffer;
    kernel::static_queue::Buffer< uint8_t, max_queue_elements> usart_tx_buffer;

    kernel::init();

    // Create receive and transmit queues with static buffers.
    bool queue_created = kernel::static_queue::create( g_shared_data.m_usart_rx_queue, usart_rx_buffer);

    if ( false == queue_created)
    {
        kernel::hardware::debug::print( "\nFailed to create RX queue.\n");
        kernel::hardware::debug::setBreakpoint();
    }

    queue_created = kernel::static_queue::create( g_shared_data.m_usart_tx_queue, usart_tx_buffer);

    if ( false == queue_created)
    {
        kernel::hardware::debug::print( "\nFailed to create TX queue.\n");
        kernel::hardware::debug::setBreakpoint();
    }

    // Create startup task used to initialize HW.
    kernel::task::create(
        startup_task,
        kernel::task::Priority::Low,
        nullptr,
        &g_shared_data
    );

    // Create suspended worker task.
    kernel::task::create(
        worker_task,
        kernel::task::Priority::Low,
        &g_shared_data.m_worker_task,
        &g_shared_data,
        true
    );

    kernel::start();

    for(;;);
}