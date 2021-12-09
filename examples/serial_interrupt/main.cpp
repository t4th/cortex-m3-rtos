// This is on target example, but you can still use Keil simulator and set
// USART interrupts manually to pending state from within NVIC peripheral.

// Example: Echo USART 1 input and echo it also to ITM trace builit-in in debugger.
//          Under/overflow errors are not handled.

#include <kernel.hpp>

#include "gpio.hpp"

template< size_t StringSize>
void print( kernel::Handle & a_queue, const char ( &a_string)[ StringSize])
{
    size_t i = 0U;

    // Skip end-of-string sign.
    while ( '\0' != a_string[ i] && i < StringSize)
    {
        uint8_t byte_to_send = static_cast< uint8_t>( a_string[ i]);

        bool byte_sent = kernel::static_queue::send( a_queue, byte_to_send);

        // Stay in the loop until all is sent.
        if ( false == byte_sent)
        {
            kernel::hardware::debug::print( "Tx queue full.\n");

            // Enable Tx interrupt to free queue.
            USART1->CR1 |= USART_CR1_TXEIE;
        }
        else
        {
            ++i;
        }
    }

    // Enable Usart Tx interrupt.
    USART1->CR1 |= USART_CR1_TXEIE;
}

extern "C"
{
    // Note: Overrun and other errors are not handled.
    void USART1_IRQHandler()
    {
        kernel::Handle usart_rx_queue{};
        kernel::Handle usart_tx_queue{};

        // Ignore error checking.
        ( void) kernel::static_queue::open( usart_rx_queue, "RX queue");
        ( void) kernel::static_queue::open( usart_tx_queue, "TX queue");

        volatile uint16_t status_register = USART1->SR;

        // Receive handler.
        if ( status_register & USART_SR_RXNE)
        {
            uint8_t received_byte = USART1->DR & 0xFFU;

            bool byte_sent = kernel::static_queue::send( usart_rx_queue, received_byte);

            if ( false == byte_sent)
            {
                kernel::hardware::debug::print( "Rx queue full.\n");
            }

            USART1->SR &= ~USART_SR_RXNE;
        }
    
        // Transmit handler.
        if ( status_register & USART_SR_TXE)
        {
            uint8_t char_to_send{};

            // Flush Tx queue.
            while ( true)
            {
                bool byte_received = kernel::static_queue::receive(
                        usart_tx_queue,
                        char_to_send
                    );

                if ( true == byte_received)
                {
                    USART1->DR = char_to_send;
                    
                    // Wait for Transmit Complete.
                    while ( !(USART1->SR & USART_SR_TC));
                }
                else
                {
                    break;
                }
            };

            USART1->CR1 &= ~USART_CR1_TXEIE;
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
    const uint32_t  usart_baudrate_115200{ core_frequency / 115200U};

    USART1->BRR = static_cast< uint16_t>( usart_baudrate_115200);
    USART1->CR1 |=  USART_CR1_RXNEIE   // Enable rx ready interrupt.
                |   USART_CR1_RE       // Enable rx.
                |   USART_CR1_TE;      // Enable tx.

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
    constexpr size_t response_buffer_size = 64U;
    char response_buffer[ response_buffer_size];
    
    kernel::Handle usart_rx_queue{};
    kernel::Handle usart_tx_queue{};
    
    // Ignore error checking.
    ( void) kernel::static_queue::open( usart_rx_queue, "RX queue");
    ( void) kernel::static_queue::open( usart_tx_queue, "TX queue");

    print( usart_tx_queue, "Worker task started.\n");

    while( true)
    {
        // Wait until at least one element is available in queue.
        auto result = kernel::sync::waitForSingleObject( usart_rx_queue);

        if ( kernel::sync::WaitResult::ObjectSet == result)
        {
            int response_buffer_index = 0;
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
                    response_buffer[ response_buffer_index] = received_byte;
                }
                else
                {
                    // Queue is empty.
                    response_buffer[ response_buffer_index] = '\0';
                    print( usart_tx_queue, response_buffer);
                    break;
                }
                ++response_buffer_index;
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
    constexpr size_t max_queue_elements{ 64U};

    kernel::static_queue::Buffer< uint8_t, max_queue_elements> usart_rx_buffer;
    kernel::static_queue::Buffer< uint8_t, max_queue_elements> usart_tx_buffer;
    
    kernel::Handle rx_queue{};
    kernel::Handle tx_queue{};

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