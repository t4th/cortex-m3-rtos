
#include "kernel_config.h"
#include "kernel_types.h"
#include "kernel_api.h"
#include "arbiter.h"
#include "kernel.h"
// hw specyfic
#include <stm32f10x.h>

 // DUI0552A_cortex_m3_dgug

// kernel status flags
#define KERNEL_EN           0x1
#define SWITCH_REQUESTED    0x2

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

typedef struct thread_s
{
    uint32_t            sp;
    context_t           context;
    stack_t             stack;
    task_priority_t   priority;
    task_state_t      state;
    task_routine_t    routine;
} thread_t;

// --------- kernel --------------------

typedef struct kernel_s
{
    task_handle_t    current_thread;
    task_handle_t    next_thread;

    time_t      time_ms;
    uint32_t    status; // sync with systick

    uint32_t    user_thread_count;
    int         thread_data_pool_status[MAX_USER_THREADS]; // might use .routine instead
    thread_t    thread_data_pool[MAX_USER_THREADS];
    thread_t    idle_task; // always ready
} kernel_t;

// local memory
volatile kernel_t g_kernel;

arbiter_t g_arbiter; // todo: volatile



// public API
static void thread_proc(void);
void thread_idle(void);
void arbiter_start(void);

// private

// body


void printErrorMsg(const char * errMsg)
{
   while(*errMsg != '\0'){
      ITM_SendChar(*errMsg);
      ++errMsg;
   }
      ITM_SendChar('\n');
}

time_t GetTime(void)
{
    // return atomic word
    return g_kernel.time_ms;
}

int CreateTask(task_routine_t _routine, task_priority_t _priority, task_handle_t * _handle)
{
    int i;
    int ret = 0;
    
    __disable_irq();
    // todo: sanity for kernel init and args

    if (g_kernel.user_thread_count < MAX_USER_THREADS)
    {
        // find empty spot in thread pool
        // skip IDLE_TASK_ID as it takes index 0
        for (i = 0; i < MAX_USER_THREADS; i++) {
            if (0 == g_kernel.thread_data_pool_status[i]) {
                break;
            }
        }

        // todo: set stack to default or skip init if it is fist time init
        // init new thread if found empty slot
        g_kernel.thread_data_pool_status[i] = 1;
        g_kernel.thread_data_pool[i].routine = _routine;
        g_kernel.thread_data_pool[i].priority = _priority;
        if (_handle) *_handle = i;
        
        // add task to arbiter
        Arbiter_AddTask(&g_arbiter, _priority, i);
        
    } else {
        ret = 1; // no more free task space
    }

    __enable_irq();
    return ret;
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
    
    // init thread pool table
    for (thread_i = 0; thread_i< MAX_USER_THREADS; thread_i++)
    {
        g_kernel.thread_data_pool[thread_i].sp = (uint32_t)&g_kernel.thread_data_pool[thread_i].stack.data[STACK_SIZE - 8]; // last element
        
        g_kernel.thread_data_pool[thread_i].stack.data[STACK_SIZE - 8] = 0xcdcdcdcd; // r0
        g_kernel.thread_data_pool[thread_i].stack.data[STACK_SIZE - 7] = 0xcdcdcdcd; // r1
        g_kernel.thread_data_pool[thread_i].stack.data[STACK_SIZE - 6] = 0xcdcdcdcd; // r2
        g_kernel.thread_data_pool[thread_i].stack.data[STACK_SIZE - 5] = 0xcdcdcdcd; // r3
        g_kernel.thread_data_pool[thread_i].stack.data[STACK_SIZE - 4] = 0;//0xabababab; // r12
        g_kernel.thread_data_pool[thread_i].stack.data[STACK_SIZE - 3] = 0; // lr r14
        g_kernel.thread_data_pool[thread_i].stack.data[STACK_SIZE - 2] = (uint32_t)thread_proc; // return address, pc+1
        g_kernel.thread_data_pool[thread_i].stack.data[STACK_SIZE - 1] = 0x01000000; // xPSR
    }
    
    // init IDLE task
    g_kernel.idle_task.sp = (uint32_t)&g_kernel.idle_task.stack.data[STACK_SIZE - 8]; // last element
    
    g_kernel.idle_task.stack.data[STACK_SIZE - 8] = 0xcdcdcdcd; // r0
    g_kernel.idle_task.stack.data[STACK_SIZE - 7] = 0xcdcdcdcd; // r1
    g_kernel.idle_task.stack.data[STACK_SIZE - 6] = 0xcdcdcdcd; // r2
    g_kernel.idle_task.stack.data[STACK_SIZE - 5] = 0xcdcdcdcd; // r3
    g_kernel.idle_task.stack.data[STACK_SIZE - 4] = 0;//0xabababab; // r12
    g_kernel.idle_task.stack.data[STACK_SIZE - 3] = 0; // lr r14
    g_kernel.idle_task.stack.data[STACK_SIZE - 2] = (uint32_t)thread_idle; // return address, pc+1
    g_kernel.idle_task.stack.data[STACK_SIZE - 1] = 0x01000000; // xPSR

    // sort all threads in thread pool
    if (0 == g_kernel.user_thread_count) {
        g_kernel.current_thread = IDLE_TASK_ID; // 0 idle task ID
        g_kernel.next_thread = IDLE_TASK_ID;
    }
    else
    {
        // call arbiter
        //  - set current and next
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
            g_kernel.current_thread = INVALID_HANDLE;
        }
        else
        {
            // set first task
            g_kernel.thread_data_pool[h].sp = (uint32_t)&g_kernel.thread_data_pool[h].stack.data[STACK_SIZE - 1]; // last element
            __set_PSP(g_kernel.thread_data_pool[h].sp); // initial psp
            g_kernel.current_thread = h;
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
//    g_kernel.thread_data_pool[0].routine(); // run
    
    thread_proc();
}

// private api
static void thread_finished(void)
{
    // get current task
    // remove task from data pool
    // call arbiter to get next task
}

static void thread_proc(void)
{
    //p->routine();
    
    if (IDLE_TASK_ID == g_kernel.current_thread) {
        g_kernel.idle_task.routine();
    } else {
        g_kernel.thread_data_pool[g_kernel.current_thread].routine();
    }
    thread_finished();
}

void thread_idle(void)
{
    while (1);
}


// sys_tick
void arbiter_run(void)
{
    // get current task priority

    // check high priority
    // check medium priority
    // check low priority


}


volatile uint32_t * current_task_context = 0;
volatile uint32_t * next_task_context = 0;

void SysTick_Handler(void)
{
    static unsigned old_time = 0;
    __disable_irq();
    g_kernel.time_ms++;
    
    if (g_kernel.time_ms - old_time > 10)
    {
        old_time = g_kernel.time_ms;
        if ((g_kernel.status & KERNEL_EN) )
        {
            // if switch requested
            // if same priority list has 'next'
            // 1. get current task prio
            // 2. get current task arbiter position
            // 3. find 'next'
            {
                int prio = g_kernel.thread_data_pool[g_kernel.current_thread].priority; // 1. get current task prio
                
                if (g_arbiter.task_list[prio].count > 1 && // dont context switch if there is only 1 task
                    g_arbiter.task_list[prio].current < g_arbiter.task_list[prio].count) 
                {
                    task_handle_t next_handle;
                    
                    g_arbiter.task_list[prio].next += 1; // init is current = next, next ++
                    
                    if (g_arbiter.task_list[prio].next == g_arbiter.task_list[prio].count) { // next == count -> next = 0
                        g_arbiter.task_list[prio].next = 0; // go to the first task in prio list. todo: this will blow up
                    }
                    
                    g_arbiter.task_list[prio].current = g_arbiter.task_list[prio].next;
                    next_handle = g_arbiter.task_list[prio].list[g_arbiter.task_list[prio].next];
                    
                    g_kernel.next_thread = next_handle;
                    g_kernel.status |= SWITCH_REQUESTED;
                } // else g_kernel.status &= ~SWITCH_REQUESTED;
            }
            
            if (g_kernel.status & SWITCH_REQUESTED)
            {
                g_kernel.status &= ~SWITCH_REQUESTED;
                
                g_kernel.thread_data_pool[g_kernel.current_thread].sp = __get_PSP(); // store current sp
                current_task_context = (volatile uint32_t *)&g_kernel.thread_data_pool[g_kernel.current_thread].context;
            
                next_task_context = (volatile uint32_t *)&g_kernel.thread_data_pool[g_kernel.next_thread].context;
                __set_PSP( g_kernel.thread_data_pool[g_kernel.next_thread].sp); // set next sp
                
                g_kernel.current_thread = g_kernel.next_thread;
                                
                SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
            }
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
