/*
File: gpio.h
Description: STM32 GPIO peripheral library
Author: Adam Orcholski, tath@o2.pl, www.tath.eu
Log (day/month/year):
- (07.04.2013) Initial
- (19.07.2015) Refactored
Example:
    1. Configure Pin 9 of GPIOA as 50 MHz push-pull output:
    Gpio_SetPinAsOutput(GPIOA, GPIO_PIN9, GPIO_SPEED_50MHZ, GPIO_AF_PUSH_PULL);
    Gpio_SetOutputPin(GPIOA, GPIO_PIN9);
    Gpio_ClearOutputPin(GPIOA, GPIO_PIN9);
    uint8_t pinValue = 0;
    pinValue = Gpio_GetOutputPinValue(GPIOA, GPIO_PIN9);
    
    2. Configure Pin 9 of GPIOA as floating input:
    Gpio_SetPinAsInput(GPIOA, GPIO_PIN_t pin, GPIO_FLOATING_IN);
    uint8_t pinValue = 0;
    pinValue = Gpio_GetInputPinValue(GPIOA, GPIO_PIN9);
Note:
- pGpio argument should be one of predefined values:
  GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG
*/

#ifndef _GPIO_DEF_
#define _GPIO_DEF_

#include <stdint.h>
#include <stm32f10x.h>

/* Types */
typedef enum GPIO_OUTPUT_SPEED_t
{
    GPIO_SPEED_10MHZ = 0x01,   /* Max speed 10 MHz */
    GPIO_SPEED_2MHZ  = 0x02,   /* Max speed 2 MHz */
    GPIO_SPEED_50MHZ = 0x03    /* Max speed 50 MHz */
} GPIO_OUTPUT_SPEED_t;

typedef enum GPIO_OUTPUT_MODE_t
{
    GPIO_GP_PUSH_PULL  = 0x00,   /* General purpose output push-pull */
    GPIO_GP_OPEN_DRAIN = 0x01,   /* General purpose output Open-drain */
    GPIO_AF_PUSH_PULL  = 0x02,   /* Alternate function output Push-pull */
    GPIO_AF_OPEN_DRAIN = 0x03    /* Alternate function output Open-drain */
} GPIO_OUTPUT_MODE_t;

typedef enum GPIO_INPUT_MODE_t
{
    GPIO_ANALOG_MODE = 0x00,    /* Analog mode */
    GPIO_FLOATING_IN = 0x01,    /* Floating input */
    GPIO_PU_PD_INPUT = 0x02     /* Input with pull-up / pull-down */
} GPIO_INPUT_MODE_t;

typedef enum GPIO_PIN_t
{
    GPIO_PIN0 = 0,
    GPIO_PIN1 = 1,
    GPIO_PIN2 = 2,
    GPIO_PIN3 = 3,
    GPIO_PIN4 = 4,
    GPIO_PIN5 = 5,
    GPIO_PIN6 = 6,
    GPIO_PIN7 = 7,
    GPIO_PIN8 = 8,
    GPIO_PIN9 = 9,
    GPIO_PIN10 = 10,
    GPIO_PIN11 = 11,
    GPIO_PIN12 = 12,
    GPIO_PIN13 = 13,
    GPIO_PIN14 = 14,
    GPIO_PIN15 = 15
} GPIO_PIN_t;

/* Interface function declarations */
void    Gpio_SetPinAsInput(GPIO_TypeDef * const pGpio, GPIO_PIN_t pin, GPIO_INPUT_MODE_t mode);
uint8_t Gpio_GetInputPinValue(GPIO_TypeDef * const pGpio, GPIO_PIN_t pin);

void    Gpio_SetPinAsOutput(GPIO_TypeDef * const pGpio, GPIO_PIN_t pin, GPIO_OUTPUT_SPEED_t speed, GPIO_OUTPUT_MODE_t mode);
void    Gpio_SetOutputPin(GPIO_TypeDef * const pGpio, GPIO_PIN_t pin);
void    Gpio_ClearOutputPin(GPIO_TypeDef * const pGpio, GPIO_PIN_t pin);
uint8_t Gpio_GetOutputPinValue(GPIO_TypeDef * const pGpio, GPIO_PIN_t pin);

#endif
