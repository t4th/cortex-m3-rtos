#include <stm32f10x.h>

#include "gpio.h"

#include "kernel.h"
#include "kernel_api.h"

void HI_prio_task(void)
{
    time_ms_t current_time = 0;
    time_ms_t old_time = GetTime();
    printErrorMsg("HI prio spam start");
    
    do
    {
        current_time = GetTime();
        
        if (current_time - old_time > 1000u) // 1ms * 1000->1s
        {
            printErrorMsg("HI prio spam stop");
            Sleep(5000);
            printErrorMsg("HI prio ends");
            break; // thread finished
        }
    } while (1);
}

void led(GPIO_PIN_t pin, const char * txt, time_ms_t time)
{
    time_ms_t current_time = 0;
    time_ms_t old_time = 0;
    int prev_state = 0;
    do
    {
        current_time = GetTime();
        
        if (current_time - old_time > time) // 1ms * 1000->1s
        {
            printErrorMsg(txt);
            old_time = current_time;
            if (prev_state == 0)
            {
                prev_state = 1;
                Gpio_SetOutputPin(GPIOF, pin);
            }
            else
            {
                handle_t handle;
                Gpio_ClearOutputPin(GPIOF, pin);
                prev_state = 0;
                Sleep(time);
                {
                    static int once = 0;
                    if (once == 0) {
                        once = 1;
                        CreateTask(HI_prio_task, T_HIGH, &handle, FALSE);
                    }
                }
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
                printErrorMsg("GPIO_PIN6 <- Medium 1");
                break;
            case GPIO_PIN7:
                printErrorMsg("GPIO_PIN7 <----");
                break;
            case GPIO_PIN8:
                printErrorMsg("GPIO_PIN8 <- Medium 2");
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
    led(GPIO_PIN7,"GPIO_PIN7 <--- low 1", 300);
}

void thread2(void)
{
    led2(GPIO_PIN8, 300);
}

void thread3(void)
{
    //led2(GPIO_PIN9, 200);
    led(GPIO_PIN9,"GPIO_PIN9 <--- Low 2", 500);
}

void thread4(void)
{
    //led2(GPIO_PIN9, 200);
    led(GPIO_PIN9,"GPIO_PIN9 <--- Low 3", 400);
}

void thread5(void)
{
    //led2(GPIO_PIN9, 200);
    led(GPIO_PIN9,"GPIO_PIN9 <--- Low 4", 250);
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
    CreateTask(thread3, T_LOW, 0, FALSE);
    CreateTask(thread4, T_LOW, 0, FALSE);
    CreateTask(thread5, T_LOW, 0, FALSE);
    
    kernel_start();
    
    while (1);
}
