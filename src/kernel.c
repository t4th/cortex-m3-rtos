
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
#define KERNEL_EN                  0x1
#define KERNEL_SWITCH_REQUESTED    0x2
#define KERNEL_SWITCH_NO_STORE     0x4

// --------------- task ------------
struct stack_s
{
    //uint32_t size;
    uint32_t data[TASK_STACK_SIZE];
};

struct context_s
{
    uint32_t    r4, r5, r6, r7,
                r8, r9, r10, r11;
};

struct task_s
{
    uint32_t                 sp;
    struct context_s         context;
    struct stack_s           stack;
    enum task_priority_e     priority;
    enum task_state_e        state;
    task_routine_t           routine;
    BOOL                     inside_lock; // if set (cs), task switch is not available
};

// --------- kernel --------------------

struct kernel_s
{
    task_handle_t       current_task;
    task_handle_t       next_task;

    time_ms_t           time;
    time_ms_t           interval;
    uint32_t            status; // sync with systick
        
    uint32_t            user_task_count;
    int                 task_data_pool_status[MAX_USER_TASKS]; // might use .routine instead
    struct task_s       task_data_pool[MAX_USER_TASKS];
    
    arbiter_t   arbiter;
    
    /* system objects */
};

// local memory
volatile struct kernel_s g_kernel;

// private API
static void task_proc(void);
static void task_idle(void);
static void task_finished(void);
static void init_task(volatile struct task_s * task);

// body

enum switch_type_e
{
    ISSUE_SWITCH_NORMAL = 0,
    ISSUE_SWITCH_NO_STORE
};

static void kernel_issue_switch(enum switch_type_e no_store)
{
    g_kernel.next_task = Arbiter_GetHigestPrioTask(g_kernel.arbiter);
    
    if (INVALID_HANDLE == g_kernel.next_task) {
        // should never be empty, since IDLE task is there at lowest priority
        while(1);
    }        
    
    if (no_store) {
        g_kernel.status |= KERNEL_SWITCH_NO_STORE;
    }
        
    g_kernel.status |= KERNEL_SWITCH_REQUESTED;
    
    if (!(SCB->SHCSR & SCB_SHCSR_SYSTICKACT_Msk)) {
        SCB->ICSR |= SCB_ICSR_PENDSTSET_Msk;
    }
}

void printErrorMsg(const char * errMsg)
{
    __disable_irq();
    {
       while(*errMsg != '\0') {
          ITM_SendChar(*errMsg);
          ++errMsg;
       }
       ITM_SendChar('\n');
   }
    __enable_irq();
}

time_ms_t GetTime(void)
{
    return g_kernel.time; // return atomic word
}


void Sleep(time_ms_t delay)
{
    // todo: sanity
    __disable_irq();
    {
        timer_handle_t  timer;
        handle_t        handle;
        
        handle  = GetHandle();
        
        // create timer and bind task to timer
        timer = CreateTimer(delay, E_PULSE, &handle);
        
        if (INVALID_HANDLE == timer) {
            while(1); // unhandled expection
        }
            
        // go to Waiting mode (remove from arbiter)
        Arbiter_RemoveTask(g_kernel.arbiter, g_kernel.task_data_pool[handle.value].priority, handle.value);
        g_kernel.task_data_pool[handle.value].state = T_TASK_WAITING;
        StartTimer(timer);
        
        kernel_issue_switch(ISSUE_SWITCH_NORMAL);
    }
    __enable_irq();
}

int CreateTask(task_routine_t _routine, enum task_priority_e _priority, handle_t * _handle, BOOL create_suspended)
{
    // todo: sanity for kernel init and args
    int ret = 0;
    
    __disable_irq();
    {
        printErrorMsg("Created task");
        
        if (g_kernel.user_task_count < MAX_USER_TASKS)
        {
            // find empty spot in thread pool
            int task_id;
            for (task_id = 0; task_id < MAX_USER_TASKS; task_id++) {
                if (0 == g_kernel.task_data_pool_status[task_id]) {
                    break;
                }
            }

            // set stack to default or skip init if it is fist time init
            if (g_kernel.status & KERNEL_EN) {
                init_task(&g_kernel.task_data_pool[task_id]);
            }
            
            // init new thread if found empty slot
            g_kernel.task_data_pool_status[task_id] = 1;
            g_kernel.user_task_count++;
            g_kernel.task_data_pool[task_id].routine = _routine;
            g_kernel.task_data_pool[task_id].priority = _priority;
            if (_handle) {
                _handle->value = task_id;
                _handle->type = E_TASK;
            }
            
            if (create_suspended) {
                g_kernel.task_data_pool[task_id].state = T_TASK_SUSPENDED;
            }
            else { // add task to arbiter
                g_kernel.task_data_pool[task_id].state = T_TASK_READY;
                Arbiter_AddTask(g_kernel.arbiter, _priority, task_id);

                // if task created in run-time - force re-arbitration
                // todo: only when new task prio is higher than current task prio
                if (g_kernel.status & KERNEL_EN ) {
                    kernel_issue_switch(ISSUE_SWITCH_NORMAL);
                }
            }
        } else {
            ret = 1; // no more free task space
        }
    }
    __enable_irq();
    return ret;
}

void TerminateTask(handle_t * task)
{
    // todo: sanity
    __disable_irq();
    {
        if (task->type == E_TASK)
        {
            // get current task
            const enum task_priority_e task_to_remove_prio = g_kernel.task_data_pool[task->value].priority;
            // remove task from data pool
            g_kernel.task_data_pool_status[task->value] = 0;
            // remove task from arbiter
            Arbiter_RemoveTask(g_kernel.arbiter, task_to_remove_prio, task->value);
            // kill all related timers, events, etc
            {
                RemoveTimer(task);
            }
            
            if (g_kernel.user_task_count > 0) {
                g_kernel.user_task_count --;
            }
            
            // request switch, but without storing current context (since its not existing anymore)
            kernel_issue_switch(ISSUE_SWITCH_NO_STORE);
        
            printErrorMsg("task finished");
        }
    }
    __enable_irq();
}

// wake, as set to READY state (add to arbiter)
void ResumeTask(task_handle_t task)
{
    __disable_irq();
    {
        // todo: sanity - if task is already added, do nothing!
        Arbiter_AddTask(g_kernel.arbiter, g_kernel.task_data_pool[task].priority, task);
        g_kernel.task_data_pool[task].state = T_TASK_READY;
        
        kernel_issue_switch(ISSUE_SWITCH_NORMAL);
    }
    __enable_irq();
}

void interrupt_occurred(int id)
{
    // ,,,
    // call arbiter
}

static void init_task(volatile struct task_s * task)
{
    task->sp = (uint32_t)&task->stack.data[TASK_STACK_SIZE - 8]; // last element
    task->stack.data[TASK_STACK_SIZE - 8] = 0xcdcdcdcd; // r0
    task->stack.data[TASK_STACK_SIZE - 7] = 0xcdcdcdcd; // r1
    task->stack.data[TASK_STACK_SIZE - 6] = 0xcdcdcdcd; // r2
    task->stack.data[TASK_STACK_SIZE - 5] = 0xcdcdcdcd; // r3
    task->stack.data[TASK_STACK_SIZE - 4] = 0;//0xabababab; // r12
    task->stack.data[TASK_STACK_SIZE - 3] = 0; // lr r14
    task->stack.data[TASK_STACK_SIZE - 2] = (uint32_t)task_proc;
    task->stack.data[TASK_STACK_SIZE - 1] = 0x01000000; // xPSR
}

void kernel_init(void)
{
    __disable_irq();
    {
        Arbiter_Init(&g_kernel.arbiter);
        
        ITM->TCR |= ITM_TCR_ITMENA_Msk;  // ITM enable
        ITM->TER = 1UL;                  // ITM Port #0 enable
        
        g_kernel.interval = 10; // 10ms round-robin
        
        // init thread pool table
        {
            int i = 0;
            for (i = 0; i< MAX_USER_TASKS; i++) {
                init_task(&g_kernel.task_data_pool[i]);
            }
        }
        
        // create IDLE task
        {
            handle_t idle_handle;
            
            if (CreateTask(task_idle, T_LOW, &idle_handle, FALSE)) {
                while(1); // no free space during init? should never happen..
            }
            
            g_kernel.current_task = idle_handle.value;
            g_kernel.next_task = idle_handle.value;
        }
        
        printErrorMsg("init done");
    }
    __enable_irq();
}

void kernel_start(void)
{
    __disable_irq();
    {
        // setup interrupts
        NVIC_SetPriority(SysTick_IRQn, 10); // systick should be higher than pendsv
        NVIC_SetPriority(PendSV_IRQn, 12);
        
        NVIC_EnableIRQ(SysTick_IRQn);
        NVIC_EnableIRQ(PendSV_IRQn);
        
        // request switch, but without storing current context since there is none
        // set first highest priority task found
        kernel_issue_switch(ISSUE_SWITCH_NO_STORE);
        
        // run kernel
        g_kernel.status |= KERNEL_EN; // sync with sysTick, todo: this is temp. fix
    }
    __enable_irq();
}

handle_t GetHandle(void)
{
    handle_t hTask;
    __disable_irq();
    {
        hTask.value =  g_kernel.current_task;
        hTask.type = E_TASK;
    }
    __enable_irq();
    return hTask;
}

// private api
static void task_finished(void)
{
    __disable_irq();
    {
        handle_t hCurrentTask = GetHandle();
        TerminateTask(&hCurrentTask);
    }
    __enable_irq();
}

static void task_proc(void)
{
    g_kernel.task_data_pool[g_kernel.current_task].routine();

    task_finished();
}

void task_idle(void)
{
    // for now, idle task does nothing
    // todo: calculate system load
    while (1);
}

volatile uint32_t * current_task_context = 0;
volatile uint32_t * next_task_context = 0;
volatile uint32_t skip_store = 0;

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
            
            if (0 == (g_kernel.status & KERNEL_SWITCH_REQUESTED)) // if switch is not requested do arbitration
            {
                // round-robin schedule of same priority tasks
                enum task_priority_e prio = g_kernel.task_data_pool[g_kernel.current_task].priority;
                task_handle_t next_task = Arbiter_FindNext(g_kernel.arbiter, prio);
                if (INVALID_HANDLE == next_task){
                    while (1);
                }
                
                if (g_kernel.current_task != next_task)
                {
                    g_kernel.task_data_pool[g_kernel.current_task].state = T_TASK_READY;
                    g_kernel.task_data_pool[g_kernel.next_task].state = T_TASK_RUNNING;
                    g_kernel.next_task = next_task;
                    g_kernel.status |= KERNEL_SWITCH_REQUESTED;
                }
            } else { // switch was requested from kernel API
                old_time = g_kernel.time; // reset timestamp
            }
         }
            
        if (g_kernel.status & KERNEL_SWITCH_REQUESTED) {
            g_kernel.status &= ~KERNEL_SWITCH_REQUESTED;
            
            if (g_kernel.status & KERNEL_SWITCH_NO_STORE) {
                g_kernel.status &= ~KERNEL_SWITCH_NO_STORE;
                skip_store = 1;
            }
            else
            {
                if (INVALID_HANDLE == g_kernel.current_task ||
                    INVALID_HANDLE == g_kernel.next_task) {
                        while(1);//unhandled expection
                    }
                    
                g_kernel.task_data_pool[g_kernel.current_task].sp = __get_PSP(); // store current sp
                current_task_context = (volatile uint32_t *)&g_kernel.task_data_pool[g_kernel.current_task].context;
            }
            
            next_task_context = (volatile uint32_t *)&g_kernel.task_data_pool[g_kernel.next_task].context;
            __set_PSP( g_kernel.task_data_pool[g_kernel.next_task].sp);             // set next sp
            g_kernel.task_data_pool[g_kernel.next_task].state = T_TASK_RUNNING;     // update tasks state
            
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
    extern skip_store;
    
    CPSID I
    
    ldr r2, =skip_store; if non-zero skip store_current_task
    ldr r0, [r2] ; dereference pointer
    cbnz r0, load_next_task;
    
store_current_task
    ldr r0, =current_task_context;
    ldr r1, [r0]
    stm r1, {r4-r11};
    
load_next_task
    ldr r0, =next_task_context;
    ldr r1, [r0]
    ldm r1, {r4-r11};
    
    ; set skip_store to 0
    mov r0, #0
    nop
    str r0, [r2]
    
    ldr r0, =0xFFFFFFFD ; return to thread mode. Without this PendSV would return to SysTick losing current thread state along the way.
    CPSIE I
    bx r0
}
