
#include "kernel_config.h"
#include "kernel_types.h"
#include "kernel_api.h"
#include "arbiter.h"
#include "timer.h"
#include "kernel.h"
// hw specyfic
#include <stm32f10x.h>

 // DUI0552A_cortex_m3_dgug

// kernel status flags
#define KERNEL_EN           0x1
#define SWITCH_REQUESTED    0x2
#define SYSTEM_IDLE_ON      0x4

// --------------- task ------------
typedef struct stack_s
{
    //uint32_t size;
    uint32_t data[STACK_SIZE];
} stack_t;

typedef struct context_s
{
    uint32_t    r4, r5, r6, r7,
                r8, r9, r10, r11;
} context_t;

typedef struct task_s
{
    uint32_t          sp;
    context_t         context;
    stack_t           stack;
    task_priority_t   priority;
    task_state_t      state;
    task_routine_t    routine;
} task_t;

typedef struct event_s
{
    event_state_t state;
    
} event_t;

// --------- kernel --------------------

typedef struct kernel_s
{
    task_handle_t    current_task;
    task_handle_t    next_task;

    time_ms_t   time;
    time_ms_t   interval;
    uint32_t    status; // sync with systick

    uint32_t    user_task_count;
    int         task_data_pool_status[MAX_USER_THREADS]; // might use .routine instead
    task_t      task_data_pool[MAX_USER_THREADS];
    task_t      idle_task; // always ready
} kernel_t;

// local memory
volatile kernel_t g_kernel;

arbiter_t g_arbiter; // todo: volatile

// public API
static void task_proc(void);
void task_idle(void);

// private

// body
static void kernel_issue_switch(void)
{
    g_kernel.next_task = Arbiter_GetHigestPrioTask(&g_arbiter);
    
    if (INVALID_HANDLE == g_kernel.next_task) {
        g_kernel.next_task = IDLE_TASK_ID;
        g_kernel.status |= SYSTEM_IDLE_ON;
    }        
    
    g_kernel.status |= SWITCH_REQUESTED;
    
    if (!(SCB->SHCSR & SCB_SHCSR_SYSTICKACT_Msk)) {
        SCB->ICSR |= SCB_ICSR_PENDSTSET_Msk;
    }
}

void printErrorMsg(const char * errMsg)
{
    __disable_irq();
   while(*errMsg != '\0') {
      ITM_SendChar(*errMsg);
      ++errMsg;
   }
   ITM_SendChar('\n');
    __enable_irq();
}

time_ms_t GetTime(void)
{
    // return atomic word
    return g_kernel.time;
}


void Sleep(time_ms_t delay)
{
    // create timer
    // bind task to timer
    // go to Waiting mode (remove from arbiter)
    timer_handle_t  timer;
    handle_t        handle;
    task_handle_t   hTask;
    
    __disable_irq();
    hTask = g_kernel.current_task;
    
    handle.handle = g_kernel.current_task;
    handle.type = E_TASK;
    
    timer = CreateTimer(delay, E_PULSE, &handle);
    
    if (INVALID_HANDLE == timer) {
        while(1); // unhandled expection
    }
        
    // todo: sanity
    Arbiter_RemoveTask(&g_arbiter, g_kernel.task_data_pool[hTask].priority, hTask);
    g_kernel.task_data_pool[hTask].state = T_TASK_WAITING;
    StartTimer(timer);
    
    kernel_issue_switch();
    
    __enable_irq();
}

int CreateTask(task_routine_t _routine, task_priority_t _priority, handle_t * _handle, BOOL create_suspended)
{
    int h; // h as handle
    int ret = 0;
    
    __disable_irq();
    // todo: sanity for kernel init and args

    if (g_kernel.user_task_count < MAX_USER_THREADS)
    {
        // find empty spot in thread pool
        // skip IDLE_TASK_ID as it takes index 0
        for (h = 0; h < MAX_USER_THREADS; h++) {
            if (0 == g_kernel.task_data_pool_status[h]) {
                break;
            }
        }

        // todo: set stack to default or skip init if it is fist time init
        // init new thread if found empty slot
        g_kernel.task_data_pool_status[h] = 1;
        g_kernel.user_task_count++;
        g_kernel.task_data_pool[h].routine = _routine;
        g_kernel.task_data_pool[h].priority = _priority;
        if (_handle) {
            _handle->handle = h;
            _handle->type = E_TASK;
        }
        
        if (create_suspended) { // // add task to arbiter
            g_kernel.task_data_pool[h].state = T_TASK_SUSPENDED;
        }
        else {
            g_kernel.task_data_pool[h].state = T_TASK_READY;
            Arbiter_AddTask(&g_arbiter, _priority, h);

            // if task created in run-time force re-arbitration
            if (g_kernel.status & KERNEL_EN ) {
                kernel_issue_switch();
            }
        }
    } else {
        ret = 1; // no more free task space
    }

    __enable_irq();
    return ret;
}

void TerminateThread(void)
{
    // pretty much call
    // thread_finished
}

// wake, as set to READY state (add to arbiter)
void ResumeTask(task_handle_t task)
{
    __disable_irq();
    
    // todo: sanity
    Arbiter_AddTask(&g_arbiter, g_kernel.task_data_pool[task].priority, task);
    g_kernel.task_data_pool[task].state = T_TASK_READY;
    
    kernel_issue_switch();
    
    __enable_irq();
}

void kernel_tick(void)
{
    
}

void interrupt_occurred(int id)
{
    // ,,,
    // call arbiter
}

void kernel_init(void)
{
    int thread_i = 0;
    
    __disable_irq();
    
    Arbiter_Init(&g_arbiter);
    
    ITM->TCR |= ITM_TCR_ITMENA_Msk;   // ITM enable
    ITM->TER = 1UL;                  // ITM Port #0 enable
    
    g_kernel.interval = 10; // 10ms round-robin
    
    // init thread pool table
    for (thread_i = 0; thread_i< MAX_USER_THREADS; thread_i++)
    {
        g_kernel.task_data_pool[thread_i].sp = (uint32_t)&g_kernel.task_data_pool[thread_i].stack.data[STACK_SIZE - 8]; // last element
        
        g_kernel.task_data_pool[thread_i].stack.data[STACK_SIZE - 8] = 0xcdcdcdcd; // r0
        g_kernel.task_data_pool[thread_i].stack.data[STACK_SIZE - 7] = 0xcdcdcdcd; // r1
        g_kernel.task_data_pool[thread_i].stack.data[STACK_SIZE - 6] = 0xcdcdcdcd; // r2
        g_kernel.task_data_pool[thread_i].stack.data[STACK_SIZE - 5] = 0xcdcdcdcd; // r3
        g_kernel.task_data_pool[thread_i].stack.data[STACK_SIZE - 4] = 0;//0xabababab; // r12
        g_kernel.task_data_pool[thread_i].stack.data[STACK_SIZE - 3] = 0; // lr r14
        g_kernel.task_data_pool[thread_i].stack.data[STACK_SIZE - 2] = (uint32_t)task_proc; // return address, pc+1
        g_kernel.task_data_pool[thread_i].stack.data[STACK_SIZE - 1] = 0x01000000; // xPSR
    }
    
    // init IDLE task
    g_kernel.idle_task.sp = (uint32_t)&g_kernel.idle_task.stack.data[STACK_SIZE - 8]; // last element
    g_kernel.idle_task.routine = task_idle;
    
    g_kernel.idle_task.stack.data[STACK_SIZE - 8] = 0xcdcdcdcd; // r0
    g_kernel.idle_task.stack.data[STACK_SIZE - 7] = 0xcdcdcdcd; // r1
    g_kernel.idle_task.stack.data[STACK_SIZE - 6] = 0xcdcdcdcd; // r2
    g_kernel.idle_task.stack.data[STACK_SIZE - 5] = 0xcdcdcdcd; // r3
    g_kernel.idle_task.stack.data[STACK_SIZE - 4] = 0;//0xabababab; // r12
    g_kernel.idle_task.stack.data[STACK_SIZE - 3] = 0; // lr r14
    g_kernel.idle_task.stack.data[STACK_SIZE - 2] = (uint32_t)task_idle; // return address, pc+1
    g_kernel.idle_task.stack.data[STACK_SIZE - 1] = 0x01000000; // xPSR

    if (0 == g_kernel.user_task_count) {
        g_kernel.current_task = IDLE_TASK_ID; // 0 idle task ID
        g_kernel.next_task = IDLE_TASK_ID;
    }
    else
    {
        //  - set current and next -> done in kernel->run
    }
    __enable_irq();
    printErrorMsg("init done");
}

void kernel_start(void)
{
    __disable_irq();
    
    // set first highest priority task found as first
    {
        task_handle_t h;
        h = Arbiter_GetHigestPrioTask(&g_arbiter);

        // no user task found, set idle as default
        if (INVALID_HANDLE == h) {
            // set sp as last last element of stored sp
            g_kernel.idle_task.sp = (uint32_t)&g_kernel.idle_task.stack.data[STACK_SIZE - 1];
            __set_PSP(g_kernel.idle_task.sp); // initial psp
            g_kernel.current_task = IDLE_TASK_ID;
            g_kernel.status |= SYSTEM_IDLE_ON;
        }
        else
        {
            // set first task
            g_kernel.task_data_pool[h].sp = (uint32_t)&g_kernel.task_data_pool[h].stack.data[STACK_SIZE - 1]; // last element
            __set_PSP(g_kernel.task_data_pool[h].sp); // initial psp
            g_kernel.current_task = h;
        }
    }

    // setup interrupts
    NVIC_SetPriority(SysTick_IRQn, 10); // systick should be higher than pendsv
    NVIC_SetPriority(PendSV_IRQn, 12);
    
    NVIC_EnableIRQ(SysTick_IRQn);
    NVIC_EnableIRQ(PendSV_IRQn);
    
    // run kernel
    __enable_irq();
    __set_CONTROL(2); // sp -> psp
    g_kernel.status |= KERNEL_EN; // sync with sysTick, todo: this is temp. fix
//    g_kernel.task_data_pool[0].routine(); // run
    
    task_proc();
}

// private api
static void thread_finished(void)
{
    // get current task
    // remove task from data pool
    // call arbiter to get next task
    // kill all related timers, events, etc
}

static void task_proc(void)
{
    if (IDLE_TASK_ID != g_kernel.current_task) {
        g_kernel.task_data_pool[g_kernel.current_task].routine();
    } else {
        g_kernel.idle_task.routine();
    }
    thread_finished();
}

void task_idle(void)
{
    while (1);
}

volatile uint32_t * current_task_context = 0;
volatile uint32_t * next_task_context = 0;

void SysTick_Handler(void)
{
    static unsigned old_time = 0;
    __disable_irq();
    g_kernel.time++;
    
    if (g_kernel.status & KERNEL_EN )
    {
        RunTimers();
        
        if (g_kernel.time - old_time > g_kernel.interval)
        {
            old_time = g_kernel.time;
            if (0 == (g_kernel.status & SYSTEM_IDLE_ON))
            {
                if (0 == (g_kernel.status & SWITCH_REQUESTED)) // if switch is not requested do arbitration
                {
                    // round-robin schedule of same priority tasks
                    task_priority_t prio = g_kernel.task_data_pool[g_kernel.current_task].priority;
                    task_handle_t next_task = Arbiter_FindNext(&g_arbiter, prio);
                    if (INVALID_HANDLE == next_task){
                        while (1);
                    }
                    
                    if (g_kernel.current_task != next_task)
                    {
                        g_kernel.task_data_pool[g_kernel.current_task].state = T_TASK_READY;
                        g_kernel.task_data_pool[g_kernel.next_task].state = T_TASK_RUNNING;
                        g_kernel.next_task = next_task;
                        g_kernel.status |= SWITCH_REQUESTED;
                    }
                } else { // switch was requested from kernel API
                    old_time = g_kernel.time; // reset timestamp
                }
            } else{ // IDLE task
                if (g_kernel.next_task != IDLE_TASK_ID) {
                    g_kernel.status &= ~SYSTEM_IDLE_ON;
                    g_kernel.status |= SWITCH_REQUESTED;
                }
            }
         }
            
        if (g_kernel.status & SWITCH_REQUESTED) {
            g_kernel.status &= ~SWITCH_REQUESTED;
            
            if (IDLE_TASK_ID == g_kernel.current_task)
            {
                g_kernel.idle_task.sp = __get_PSP(); // store current sp
                current_task_context = (volatile uint32_t *)&g_kernel.idle_task.context;
            } else {
                g_kernel.task_data_pool[g_kernel.current_task].sp = __get_PSP(); // store current sp
                current_task_context = (volatile uint32_t *)&g_kernel.task_data_pool[g_kernel.current_task].context;
            }
            
            if (IDLE_TASK_ID == g_kernel.next_task)
            {
                next_task_context = (volatile uint32_t *)&g_kernel.idle_task.context;
                __set_PSP( g_kernel.idle_task.sp); // set next sp
                // update tasks state
                g_kernel.idle_task.state = T_TASK_RUNNING;
            } else {
                next_task_context = (volatile uint32_t *)&g_kernel.task_data_pool[g_kernel.next_task].context;
                __set_PSP( g_kernel.task_data_pool[g_kernel.next_task].sp); // set next sp
                // update tasks state
                g_kernel.task_data_pool[g_kernel.next_task].state = T_TASK_RUNNING;
            }
            
            g_kernel.current_task = g_kernel.next_task;
            
            SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
        }
    }
    __enable_irq();
}

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
    
    ldr r0, =0xFFFFFFFD ; return to thread mode. Without this PendSV would return to SysTick losing current thread state along the way.
    CPSIE I
    bx r0
    nop
}
