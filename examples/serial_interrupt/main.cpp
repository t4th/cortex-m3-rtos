// This is on target example, but you can still use Keil simulator and set
// USART interrupts manually to pending state from within NVIC peripheral.

// Example: Echo input string from usart 1 to ITM trace builit-in in debugger.

#include <kernel.hpp>

#include "gpio.hpp"

// TODO: implement tx queue and interrupt

void print( const char * a_text)
{
    kernel::hardware::debug::print( a_text);
}

extern "C"
{
    void USART1_IRQHandler()
    {
        kernel::Handle usart_rx_queue;
    
        // Of course most effective would be doing this just once,
        // but this is just an API example.
        bool queue_opened = kernel::static_queue::open( usart_rx_queue, "RX queue");

        if ( false == queue_opened)
        {
            kernel::hardware::debug::print( "\nFailed to open queue.\n");
            kernel::hardware::debug::setBreakpoint();
        }

        {
           volatile uint16_t status_register = USART1->SR;

            if ( status_register & USART_SR_RXNE)
            {
                uint8_t received_byte = USART1->DR & 0xFF;
            
                bool queue_not_full = kernel::static_queue::send(
                        usart_rx_queue,
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
}

// Setup hardware and resume worker task when done.
void startup_task( void * a_parameter)
{
    kernel::Handle & worker_task = *reinterpret_cast< kernel::Handle*>( a_parameter);

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

    kernel::task::resume( worker_task);
}

void worker_task( void * a_parameter)
{
    kernel::Handle usart_rx_queue;
    
    bool queue_opened = kernel::static_queue::open( usart_rx_queue, "RX queue");

    if ( false == queue_opened)
    {
        kernel::hardware::debug::print( "\nFailed to open queue.\n");
        kernel::hardware::debug::setBreakpoint();
    }

    while( true)
    {
        // Wait until at least one elements is available in queue.
        auto result = kernel::sync::waitForSingleObject( usart_rx_queue);

        if ( kernel::sync::WaitResult::ObjectSet == result)
        {
            uint8_t received_byte;

            // Flush all queue content until its empty.
            while ( true)
            {
                bool queue_not_empty = kernel::static_queue::receive(
                        usart_rx_queue,
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
    
    kernel::Handle rx_queue;
    kernel::Handle tx_queue;

    kernel::init();

    // Create receive and transmit queues with static buffers.
    bool queue_created = kernel::static_queue::create(
        rx_queue,
        usart_rx_buffer,
        "RX queue"
    );

    if ( false == queue_created)
    {
        kernel::hardware::debug::print( "\nFailed to create RX queue.\n");
        kernel::hardware::debug::setBreakpoint();
    }

    queue_created = kernel::static_queue::create(
        tx_queue,
        usart_tx_buffer,
        "TX queue"
    );

    if ( false == queue_created)
    {
        kernel::hardware::debug::print( "\nFailed to create TX queue.\n");
        kernel::hardware::debug::setBreakpoint();
    }
    
    kernel::Handle worker_task_handle;
    
    // Create suspended worker task.
    kernel::task::create(
        worker_task,
        kernel::task::Priority::Low,
        &worker_task_handle,
        nullptr,
        true
    );

    // Create startup task used to initialize HW.
    kernel::task::create(
        startup_task,
        kernel::task::Priority::Low,
        nullptr,
        &worker_task_handle
    );

    kernel::start();

    for(;;);
}