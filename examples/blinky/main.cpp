
// On target RTOS example: classic blinky.
// HW used: stm32f103ze with keil ulink2 (should work with st-link).

// 4 LEDs are connected to GPIOF, pins 6,7,8,9.
// One task is blinking 1 LED - 4 tasks for 4 LEDs.
// From human perspective it should look like
// all 4 leds are blinking at the same time.

#include <kernel.hpp>

#include "gpio.hpp"

struct LedState
{
    bool led0;
    bool led1;
    bool led2;
    bool led3;
};

// Global variable used for software debug analysis
// when using simulator mode.
volatile LedState g_ledStates;

enum class Led
{
    Number0,
    Number1,
    Number2,
    Number3
};

void setLed( Led a_led)
{
    gpio::Pin pin;

    switch ( a_led)
    {
    case Led::Number0:
        gpio::setOutputPin< gpio::Port::F, gpio::Pin::Number6>();
        g_ledStates.led0 = true;
        break;
    case Led::Number1:
        gpio::setOutputPin< gpio::Port::F, gpio::Pin::Number7>();
        g_ledStates.led1 = true;
        break;
    case Led::Number2:
        gpio::setOutputPin< gpio::Port::F, gpio::Pin::Number8>();
        g_ledStates.led2 = true;
        break;
    case Led::Number3:
        gpio::setOutputPin< gpio::Port::F, gpio::Pin::Number9>();
        g_ledStates.led3 = true;
        break;
    default:
        break;
    }
}

void clearLed( Led a_led)
{
    gpio::Pin pin;

    switch ( a_led)
    {
    case Led::Number0:
        gpio::clearOutputPin< gpio::Port::F, gpio::Pin::Number6>();
        g_ledStates.led0 = false;
        break;
    case Led::Number1:
        gpio::clearOutputPin< gpio::Port::F, gpio::Pin::Number7>();
        g_ledStates.led1 = false;
        break;
    case Led::Number2:
        gpio::clearOutputPin< gpio::Port::F, gpio::Pin::Number8>();
        g_ledStates.led2 = false;
        break;
    case Led::Number3:
        gpio::clearOutputPin< gpio::Port::F, gpio::Pin::Number9>();
        g_ledStates.led3 = false;
        break;
    default:
        break;
    }
}

void blinkLed( Led a_led, kernel::TimeMs a_time)
{
    setLed( a_led);
    kernel::task::sleep( a_time);
    clearLed( a_led);
    kernel::task::sleep( a_time);
}

void task_blink_led_0 ( void * a_parameter)
{
    while( true)
    {
        blinkLed( Led::Number0, 1000U);
    }
}

void task_blink_led_1 ( void * a_parameter)
{
    while( true)
    {
        blinkLed( Led::Number1, 1000U);
    }
}

void task_blink_led_2 ( void * a_parameter)
{
    while( true)
    {
        blinkLed( Led::Number2, 1000U);
    }
}

void task_blink_led_3 ( void * a_parameter)
{
    while( true)
    {
        blinkLed( Led::Number3, 1000U);
    }
}

int main()
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPFEN; // enable gpiof

    gpio::setPinAsOutput< gpio::Port::F, gpio::Pin::Number6, gpio::OutputSpeed::Max50MHZ, gpio::OutputMode::GeneralPurposePushPull>();
    gpio::setPinAsOutput< gpio::Port::F, gpio::Pin::Number7, gpio::OutputSpeed::Max50MHZ, gpio::OutputMode::GeneralPurposePushPull>();
    gpio::setPinAsOutput< gpio::Port::F, gpio::Pin::Number8, gpio::OutputSpeed::Max50MHZ, gpio::OutputMode::GeneralPurposePushPull>();
    gpio::setPinAsOutput< gpio::Port::F, gpio::Pin::Number9, gpio::OutputSpeed::Max50MHZ, gpio::OutputMode::GeneralPurposePushPull>();
    
    kernel::hardware::debug::print("test");
    
    kernel::init();

    kernel::task::create( task_blink_led_0, kernel::task::Priority::Low);
    kernel::task::create( task_blink_led_1, kernel::task::Priority::Low);
    kernel::task::create( task_blink_led_2, kernel::task::Priority::Low);
    kernel::task::create( task_blink_led_3, kernel::task::Priority::Low);

    kernel::start();

    for(;;);
}
