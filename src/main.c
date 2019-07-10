#include <stm32f10x.h>

#include "gpio.h"

#define STACK_SIZE 32u

typedef struct stack_s
{
    uint32_t data[STACK_SIZE];
} stack_t;

typedef void (*thread_proc_t)(void);

typedef struct context_s
{
    uint32_t r4, r5, r6, r7, r8, r9, r10, r11;
} context_t;

typedef struct thread_s
{
    uint32_t        sp; // 4
    context_t       context; // 4 * 8 = 32
    stack_t         stack; // 4 * 32 = 128
    thread_proc_t   proc; // 4
} thread_t; // 32 + 128 + 4 + 4 = 168

#define NO_THREADS 4

typedef struct kernel_s
{
    thread_t    thread[NO_THREADS];
    // 0
    // 168
    // 336
    // 504
    unsigned    current_thread;
    unsigned    next_thread;
    unsigned    time_ms;
    int         status;
} kernel_t;

volatile kernel_t kernel;

volatile __INLINE __asm void store_context(volatile context_t * p)
{
    stm r0, {r4-r11};
    bx lr;
}

volatile __INLINE __asm void load_context(volatile context_t * p)
{
    ldm r0, {r4-r11};
    bx lr;
}

__ASM void handler(void)
{
  MRS r0,PSP
  ;movs r1, =kernel
}

volatile uint32_t * current_task_context = 0;
volatile uint32_t * next_task_context = 0;

void SysTick_Handler(void)
{
    static unsigned old_time = 0;
    kernel.time_ms++;
    
    if (kernel.time_ms - old_time > 10)
    {
        old_time = kernel.time_ms;
        if (kernel.status == 1)
        {
            kernel.thread[kernel.current_thread].sp = __get_PSP(); // store current sp
            current_task_context = (volatile uint32_t *)&kernel.thread[kernel.current_thread].context;
        
            next_task_context = (volatile uint32_t *)&kernel.thread[kernel.next_thread].context;
            __set_PSP( kernel.thread[kernel.next_thread].sp); // set next sp
            
            kernel.current_thread = kernel.next_thread;
            
            kernel.next_thread ++;
            if (kernel.next_thread  >= NO_THREADS)
            {
                kernel.next_thread = 0;
            }
            SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
        }
    }
}

extern Contex_Switch(void);

void PendSV_Handler2(void)
{    
    kernel.thread[kernel.current_thread].sp = __get_PSP(); // store current sp
    store_context(&kernel.thread[kernel.current_thread].context);

    load_context(&kernel.thread[kernel.next_thread].context);
    __set_PSP( kernel.thread[kernel.next_thread].sp); // set next sp

    kernel.current_thread = kernel.next_thread;

    kernel.next_thread ++;

    if (kernel.next_thread  >= NO_THREADS)
    {
        kernel.next_thread = 0;
    }
}

volatile uint32_t * c_context1 = (volatile uint32_t *) &kernel.thread[0].context;
volatile uint32_t * c_context2 = (volatile uint32_t *) &kernel.thread[1].context;

void c_switch(void) 
{
    kernel.thread[kernel.current_thread].sp = __get_PSP(); // store current sp
    c_context1 = (volatile uint32_t *)&kernel.thread[kernel.current_thread].context;

    c_context2 = (volatile uint32_t *)&kernel.thread[kernel.next_thread].context;
    __set_PSP( kernel.thread[kernel.next_thread].sp); // set next sp
    
    kernel.current_thread = kernel.next_thread;
    
    kernel.next_thread ++;
    if (kernel.next_thread  >= NO_THREADS)
    {
        kernel.next_thread = 0;
    }
}

#if 1
__ASM void PendSV_Handler(void)
{
    extern current_task_context;
    extern next_task_context;
    
    CPSID I
    
    ldr r0, =current_task_context;
    ldr r1, [r0]
    stm r1, {r4-r11};
    
    ldr r0, =next_task_context;
    ldr r1, [r0]
    ldm r1, {r4-r11};
    
    ldr r0, =0xFFFFFFFD
    CPSIE I
    bx r0
    nop
}
#else
__ASM void PendSV_Handler(void)
{
    extern c_context1;
    extern c_context2;
    extern c_switch;
    
    ldr r14, =next;
    B c_switch;
    
next
    ldr r0, =c_context1;
    ldr r1, [r0]
    stm r1, {r4-r11};
    
    ldr r0, =c_context2;
    ldr r1, [r0]
    ldm r1, {r4-r11};
    
    ldr r0, =0xFFFFFFFD
    bx r0
    nop
}
#endif

void thread_proc(int thread_id)
{
    if (thread_id < NO_THREADS)
    {
        kernel.thread[thread_id].proc(); // run
        // clean up after thread
    }
}

void led(GPIO_PIN_t pin)
{
    unsigned current_time = 0;
    unsigned old_time = 0;
    int prev_state = 0;
    do
    {
        current_time = kernel.time_ms;
        
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
void init_kernel(void)
{
    int thread_i = 0;
    
    kernel.thread[0].proc = thread0;
    kernel.thread[1].proc = thread1;
    kernel.thread[2].proc = thread2;
    kernel.thread[3].proc = thread3;
    
    for (thread_i=0;thread_i<NO_THREADS;thread_i++)
    {
        kernel.thread[thread_i].sp =(uint32_t) &kernel.thread[thread_i].stack.data[STACK_SIZE-8]; // last element
        
        kernel.thread[thread_i].stack.data[STACK_SIZE-8] = 0xcdcdcdcd; // r0
        kernel.thread[thread_i].stack.data[STACK_SIZE-7] = 0xcdcdcdcd; // r1
        kernel.thread[thread_i].stack.data[STACK_SIZE-6] = 0xcdcdcdcd; // r2
        kernel.thread[thread_i].stack.data[STACK_SIZE-5] = 0xcdcdcdcd; // r3
        kernel.thread[thread_i].stack.data[STACK_SIZE-4] = 0;//0xabababab; // r12
        kernel.thread[thread_i].stack.data[STACK_SIZE-3] = 0; // lr r14
        kernel.thread[thread_i].stack.data[STACK_SIZE-2] = (uint32_t)kernel.thread[thread_i].proc; // return address, pc+1
        kernel.thread[thread_i].stack.data[STACK_SIZE-1] = 0x01000000; // xPSR
    }
    
    kernel.current_thread = 0;
    kernel.next_thread = 1;
    
    NVIC_SetPriority(SysTick_IRQn, 10); // systick should be higher than pendsv
    NVIC_SetPriority(PendSV_IRQn, 12);
    
    NVIC_EnableIRQ(SysTick_IRQn);
    NVIC_EnableIRQ(PendSV_IRQn);
    
    kernel.thread[0].sp =(uint32_t) &kernel.thread[0].stack.data[STACK_SIZE-1]; // last element
    __set_PSP( kernel.thread[0].sp); // initial psp
    __set_CONTROL(2); // sp -> psp
    kernel.status =1; // sync with sysTick, todo: this is temp. fix
    kernel.thread[0].proc(); // run
}

__ASM void start_kernel(void)
{
    // set sp
    ;mov R1, #2;
    ;MRS CONTROL, R1;
    ;mov R1, #0;
    ;MRS r14, R1;
}

int main()
{    
    RCC->APB2ENR |= RCC_APB2ENR_IOPFEN; // enable gpiof
    
//    Gpio_SetPinAsOutput(GPIOF, GPIO_PIN6, GPIO_SPEED_50MHZ, GPIO_GP_PUSH_PULL);
//    Gpio_SetPinAsOutput(GPIOF, GPIO_PIN7, GPIO_SPEED_50MHZ, GPIO_GP_PUSH_PULL);
//    Gpio_SetPinAsOutput(GPIOF, GPIO_PIN8, GPIO_SPEED_50MHZ, GPIO_GP_PUSH_PULL);
//    Gpio_SetPinAsOutput(GPIOF, GPIO_PIN9, GPIO_SPEED_50MHZ, GPIO_GP_PUSH_PULL);
 
    SysTick_Config(72000-1); // 1ms
    init_kernel();
    
    while (1)
    {
    }
}
