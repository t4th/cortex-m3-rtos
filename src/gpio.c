/*
File: gpio.c
Description: STM32 GPIO peripheral library
Author: Adam Orcholski, tath@o2.pl, www.tath.eu
Log (day/month/year):
- (07.04.2013) Initial
- (19.07.2015) Refactored
Note:
 - refer to official documentation to understand what is happening here
*/

#include "gpio.h"

/* Local defines */
#define    GPIO_INPUT_MODE    ((uint8_t)0x00)

/* Local function definitions */
static void Gpio_SetPinMode(GPIO_TypeDef * const pGpio, uint8_t pin, uint8_t mode, uint8_t cnf)
{
    uint32_t temp = 0;

    if (pin < 8) /* CRL register */
    {
        temp = pGpio->CRL;
        temp &= ~(0xf<<(pin*4));        /* clear fields */
        temp |=  mode << (pin*4);       /* set MODE field */
        temp |=  cnf  << ((pin*4)+2);   /* set CNF field */
        pGpio->CRL = temp;
    } 
    else /* CRH register */
    {
        pin -= 8;
        temp = pGpio->CRH;
        temp &= ~(0xf<<(pin*4));        /* clear fields */
        temp |=  mode << (pin*4);       /* set MODE field */
        temp |=  cnf  << ((pin*4)+2);   /* set CNF field */
        pGpio->CRH = temp;
    }
}

/* Public function definitions */
void Gpio_SetPinAsInput(GPIO_TypeDef * const pGpio, GPIO_PIN_t pin, GPIO_INPUT_MODE_t mode)
{
    Gpio_SetPinMode(pGpio, (uint8_t)pin, GPIO_INPUT_MODE, (uint8_t)mode);
}

uint8_t Gpio_GetInputPinValue(GPIO_TypeDef * const pGpio, GPIO_PIN_t pin)
{
     return (pGpio->IDR >> pin) & 0x1;
}

void Gpio_SetPinAsOutput(GPIO_TypeDef * const pGpio, GPIO_PIN_t pin, GPIO_OUTPUT_SPEED_t speed, GPIO_OUTPUT_MODE_t mode)
{
    Gpio_SetPinMode(pGpio, (uint8_t)pin, (uint8_t)speed, (uint8_t)mode);
}

void Gpio_SetOutputPin(GPIO_TypeDef * const pGpio, GPIO_PIN_t pin)
{
    pGpio->BSRR = (1 << pin);
}

void Gpio_ClearOutputPin(GPIO_TypeDef * const pGpio, GPIO_PIN_t pin)
{
    pGpio->BRR = (1 << pin);
}

uint8_t Gpio_GetOutputPinValue(GPIO_TypeDef * const pGpio, GPIO_PIN_t pin)
{
     return (pGpio->ODR  >> pin) & 0x1;
}
