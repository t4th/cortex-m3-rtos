// Just messing with meta programming :P

#pragma once

#include <stm32f1xx.h>

namespace gpio
{
    enum class Port
    {
        A, B, C, D, E, F, G
    };

    enum class OutputSpeed
    {
        Max10MHZ,
        Max2MHZ,
        Max50MHZ
    };

    enum class OutputMode
    {
        GeneralPurposePushPull,
        GeneralPurposeOpenDrain,
        AlternateFunctionPushPull,
        AlternateFunctionOpenDrain
    };

    enum class InputMode
    {
        Analog,
        Floating,
        PullUp,
        PullDown
    };

    enum class Pin
    {
        Number0,
        Number1,
        Number2,
        Number3,
        Number4,
        Number5,
        Number6,
        Number7,
        Number8,
        Number9,
        Number10,
        Number11,
        Number12,
        Number13,
        Number14,
        Number15
    };

    enum class PinState
    {
        Reset,
        Set
    };

    // Public user API.

    // Input Mode:
    template < Port a_port, Pin a_pin, InputMode a_mode>
    void setPinAsInput();

    template < Port a_port, Pin a_pin>
    bool getInputPinValue();

    // Output Mode:
    template < Port a_port, Pin a_pin, OutputSpeed a_speed, OutputMode a_mode>
    void setPinAsOutput();

    template < Port a_port, Pin a_pin>
    void setOutputPin();

    template < Port a_port, Pin a_pin>
    void clearOutputPin();

    template < Port a_port, Pin a_pin>
    bool getOutputPinValue();

    // Other
    template < Port a_port, Pin a_pin>
    void configureExternalInterrupt();

    // Private implementation.

    // Definitions.

    // GPIO helper functions. Should not be used externaly.
    namespace internal
    {
        constexpr GPIO_TypeDef * getBase( gpio::Port a_port);
        constexpr uint32_t getPinNumber( gpio::Pin a_pin);
        constexpr uint32_t getInputMode( gpio::InputMode a_mode);
        constexpr uint32_t getOutputMode( gpio::OutputMode a_mode);
        constexpr uint32_t getSpeed( gpio::OutputSpeed a_speed);
        void setPinMode( GPIO_TypeDef * ap_gpio, uint32_t a_pin, uint32_t a_mode, uint32_t a_config);
    }

    // Input Mode
    template < Port a_port, Pin a_pin, InputMode a_mode>
    void setPinAsInput()
    {
        GPIO_TypeDef * gpio = internal::getBase( a_port);
        uint32_t pin = internal::getPinNumber( a_pin);
        uint32_t mode = internal::getInputMode( a_mode);

        constexpr uint32_t InputMode = 0U;
        internal::setPinMode( gpio, pin, InputMode, mode);
    }

    template < Port a_port, Pin a_pin>
    bool getInputPinValue()
    {
        GPIO_TypeDef * gpio = internal::getBase( a_port);
        uint32_t pin = internal::getPinNumber( a_pin);

        uint32_t result = ( gpio->IDR  >> pin) & 0x1U;

        if ( result)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    // Output Mode
    template < Port a_port, Pin a_pin, OutputSpeed a_speed, OutputMode a_mode>
    void setPinAsOutput()
    {
        GPIO_TypeDef * gpio = internal::getBase( a_port);
        uint32_t pin = internal::getPinNumber( a_pin);
        uint32_t speed = internal::getSpeed( a_speed);
        uint32_t mode = internal::getOutputMode( a_mode);

        internal::setPinMode( gpio, pin, speed, mode);
    }

    template < Port a_port, Pin a_pin>
    void setOutputPin()
    {
        GPIO_TypeDef * gpio = internal::getBase( a_port);
        uint32_t pin = internal::getPinNumber( a_pin);

        gpio->BSRR = ( 1U << pin);
    }

    template < Port a_port, Pin a_pin>
    void clearOutputPin()
    {
        GPIO_TypeDef * gpio = internal::getBase( a_port);
        uint32_t pin = internal::getPinNumber( a_pin);

        gpio->BRR = ( 1U << pin);
    }

    template < Port a_port, Pin a_pin>
    bool getOutputPinValue()
    {
        GPIO_TypeDef * gpio = internal::getBase( a_port);
        uint32_t pin = internal::getPinNumber( a_pin);

        uint32_t result = ( gpio->ODR  >> pin) & 0x1U;

        if ( result)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    // Other
    template < Port a_port, Pin a_pin>
    void configureExternalInterrupt()
    {
        constexpr uint32_t pin = internal::getPinNumber( a_pin);
        // There are 4 EXTI_CR defined as array. Each register
        // is defined for different pins:
        // EXTICR[0] - pin 0 - 3
        // EXTICR[1] - pin 4 - 7
        // EXTICR[2] - pin 8 - 11
        // EXTICR[3] - pin 12 - 15
        // So if you write it out in binary you get:
        // 00'01 for pin 1
        // 01'00 for pin 4
        // 10'11 for pin 11
        // 11'10 for pin 14, and so on.
        // By shifting pin value >> 2 you get Exti_cr index.
        // Note: since its compile time anyway, pin if-else would make no difference :P
        constexpr uint32_t exti_control_register = pin >> 2U;
        constexpr uint32_t port = static_cast< uint32_t>( a_port);
        constexpr uint32_t bitfield_shift_value = 4U * ( pin & 0x3U);
        constexpr uint32_t bitfield_mask = 0x0FU;

        AFIO->EXTICR[ exti_control_register] &= ~( bitfield_mask << bitfield_shift_value);
        AFIO->EXTICR[ exti_control_register] |= ( port << bitfield_shift_value);
    }

    namespace internal
    {
        constexpr GPIO_TypeDef * getBase( gpio::Port a_port)
        {
            switch ( a_port)
            {
            case gpio::Port::A:
                return GPIOA;
            case gpio::Port::B:
                return GPIOB;
            case gpio::Port::C:
                return GPIOC;
            case gpio::Port::D:
                return GPIOD;
            case gpio::Port::E:
                return GPIOE;
            case gpio::Port::F:
                return GPIOF;
            case gpio::Port::G:
                return GPIOG;
            default:
                return nullptr;
            }
        }

        constexpr uint32_t getPinNumber( gpio::Pin a_pin)
        {
            return static_cast< int>( a_pin);
        }

        constexpr uint32_t getInputMode( gpio::InputMode a_mode)
        {
            constexpr uint32_t lookup[] = { 0x00U, 0x01U, 0x02U, 0x02U};

            return lookup[ static_cast< int>( a_mode)];
        }

        constexpr uint32_t getOutputMode( gpio::OutputMode a_mode)
        {
            constexpr uint32_t lookup[] = { 0x00U, 0x01U, 0x02U, 0x03U};

            return lookup[ static_cast< int>( a_mode)];
        }

        constexpr uint32_t getSpeed( gpio::OutputSpeed a_speed)
        {
            constexpr uint32_t lookup[] = { 0x01U, 0x02U, 0x03U};

            return lookup[ static_cast< int>( a_speed)];
        }

#if defined ( __CC_ARM )
    constexpr
#endif
        void setPinMode(
            GPIO_TypeDef *  ap_gpio,
            uint32_t             a_pin,
            uint32_t             a_mode,
            uint32_t             a_config
        )
        {
            constexpr uint32_t mask = 0x0FU; // Nibble mask.

            if ( a_pin < 8U) /* CRL register */
            {
                uint32_t temp = ap_gpio->CRL;
                temp &= ~( mask << ( a_pin * 4U));               /* clear fields */
                temp |=  a_mode << ( a_pin * 4U);                /* set MODE field */
                temp |=  a_config  << ( ( a_pin * 4U) + 2U);      /* set CNF field */
                ap_gpio->CRL = temp;
            } 
            else /* CRH register */
            {
                a_pin -= 8U;
                uint32_t temp = ap_gpio->CRH;
                temp &= ~( mask << ( a_pin * 4U));              /* clear fields */
                temp |=  a_mode << ( a_pin * 4U);               /* set MODE field */
                temp |=  a_config  << ( ( a_pin * 4U) + 2U);    /* set CNF field */
                ap_gpio->CRH = temp;
            }
        }
    }
}
