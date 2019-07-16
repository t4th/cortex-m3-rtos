#include <stm32f10x.h>

#include "gpio.h"

#include "kernel.h"
#include "kernel_api.h"


void led(GPIO_PIN_t pin)
{
    time_ms_t current_time = 0;
    time_ms_t old_time = 0;
    int prev_state = 0;
    do
    {
        current_time = GetTime();
        
        if (current_time - old_time > 2000u) // 1ms * 1000->1s
        {
            printErrorMsg("GPIO_PIN7 <----");
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

void led2(GPIO_PIN_t pin, time_ms_t delay)
{
    int prev_state = 0;
    do
    {
        
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
        Sleep(delay);
        switch (pin)
        {
            case GPIO_PIN6:
                printErrorMsg("GPIO_PIN6");
                break;
            case GPIO_PIN7:
                printErrorMsg("GPIO_PIN7 <----");
                break;
            case GPIO_PIN8:
                printErrorMsg("GPIO_PIN8");
                break;
            case GPIO_PIN9:
                printErrorMsg("GPIO_PIN9");
                break;
            default:
                break;
        };
        
    } while(1);
}

void thread0(void)
{
    led2(GPIO_PIN6, 200);
}

void thread1(void)
{
    led(GPIO_PIN7);
}

void thread2(void)
{
    led2(GPIO_PIN8, 800);
}

void thread3(void)
{
    led2(GPIO_PIN9, 200);
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
    
    CreateTask(thread0, T_MEDIUM, 0, FALSE);
    CreateTask(thread1, T_LOW, 0, FALSE);
    CreateTask(thread2, T_MEDIUM, 0, FALSE);
    CreateTask(thread3, T_MEDIUM, 0, FALSE);
    
    kernel_start();
    
    while (1)
    {
    }
}
