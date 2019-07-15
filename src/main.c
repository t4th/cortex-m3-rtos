#include <stm32f10x.h>

#include "gpio.h"

#include "kernel.h"
#include "kernel_api.h"


void led(GPIO_PIN_t pin)
{
    time_t current_time = 0;
    unsigned old_time = 0;
    int prev_state = 0;
    do
    {
        current_time = GetTime();
        
        if (current_time - old_time > 1000u) // 1ms * 1000->1s
        {
            old_time = current_time;
            if (prev_state == 0)
            {
                prev_state = 1;
                Gpio_SetOutputPin(GPIOF, pin);
            }
            else
            {
                Gpio_ClearOutputPin(GPIOF, pin);
                prev_state = 0;
            }
        }
    } while(1);
}

void thread0(void)
{
    led(GPIO_PIN6);
}

void thread1(void)
{
    led(GPIO_PIN7);
}

void thread2(void)
{
    led(GPIO_PIN8);
}

void thread3(void)
{
    led(GPIO_PIN9);
}

int main()
{    
    RCC->APB2ENR |= RCC_APB2ENR_IOPFEN; // enable gpiof
    
    Gpio_SetPinAsOutput(GPIOF, GPIO_PIN6, GPIO_SPEED_50MHZ, GPIO_GP_PUSH_PULL);
    Gpio_SetPinAsOutput(GPIOF, GPIO_PIN7, GPIO_SPEED_50MHZ, GPIO_GP_PUSH_PULL);
    Gpio_SetPinAsOutput(GPIOF, GPIO_PIN8, GPIO_SPEED_50MHZ, GPIO_GP_PUSH_PULL);
    Gpio_SetPinAsOutput(GPIOF, GPIO_PIN9, GPIO_SPEED_50MHZ, GPIO_GP_PUSH_PULL);
 
    SysTick_Config(72000-1); // 1ms
    
    
    kernel_init();
    
    CreateTask(thread0, T_MEDIUM, 0);
    CreateTask(thread1, T_LOW, 0);
    CreateTask(thread2, T_HIGH, 0);
    CreateTask(thread3, T_MEDIUM, 0);
    
    kernel_start();
    
    while (1)
    {
    }
}
