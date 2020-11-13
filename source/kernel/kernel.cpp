#include <kernel.hpp>

#include <hardware.hpp>

#include <scheduler.hpp>

namespace kernel::internal
{
    constexpr uint32_t DEFAULT_TIMER_INTERVAL = 10;
    
    typedef uint32_t Time_ms;
    
    struct Context
    {
        Time_ms old_time = 0U;
        Time_ms time = 0U;
        Time_ms interval = DEFAULT_TIMER_INTERVAL;
        
        kernel::task::Id m_current;
        kernel::task::Id m_next;

        bool started = false; // TODO: make status register

        // !0 'tick' will have no effect; 0 - 'tick' works normally
        // This is used as critical section
        volatile uint32_t schedule_lock = 0U;

        // data
        internal::task::Context      m_tasks;
        internal::scheduler::Context m_scheduler;
    } m_context;

    // this can be only called from handler mode (MSP stack) since it is modifying psp
    void storeContext(kernel::task::Id a_task)
    {
        const uint32_t sp = hardware::sp::get();
        internal::task::sp::set(m_context.m_tasks, a_task, sp);

        kernel::hardware::task::Context * current_task = internal::task::context::get(m_context.m_tasks, a_task);
        kernel::hardware::context::current::set(current_task);
    }

    // this can be only called from handler mode (MSP stack) since it is modifying psp
    void loadContext(kernel::task::Id a_task)
    {
        kernel::hardware::task::Context * next_task = internal::task::context::get(m_context.m_tasks, a_task);
        kernel::hardware::context::next::set(next_task);

        const uint32_t next_sp = kernel::internal::task::sp::get(m_context.m_tasks, a_task);
        kernel::hardware::sp::set(next_sp);
    }

    struct CriticalSection
    {
        CriticalSection()  {kernel::hardware::interrupt::disableAll();}
        ~CriticalSection() {kernel::hardware::interrupt::enableAll();}
    };

    // TODO: Lock/unlock need more elegant implementation.
    //       Most likely each kernel task ended with some kind of
    //       context switch would have its own SVC call.
    void lockScheduler()
    {
        ++internal::m_context.schedule_lock;
    }

    void unlockScheduler()
    {
        // TODO: thinker about removing it.
        m_context.old_time = m_context.time; // Reset Round-Robin schedule time.
        --internal::m_context.schedule_lock;
    }

    void task_routine()
    {
        kernel::task::Routine routine = internal::task::routine::get(
            m_context.m_tasks,
            m_context.m_current
        );

        void * parameter = internal::task::parameter::get(
            m_context.m_tasks,
            m_context.m_current
        );

        routine(parameter); // Call the actual task routine.

        kernel::task::terminate(m_context.m_current);
    }

    void idle_routine(void * a_parameter)
    {
        volatile int i = 0;
        while(1)
        {
            kernel::hardware::debug::print("idle\r\n");
            for (i = 0; i < 100000; i++);
            // TODO: calculate CPU load
        }
    }
}

namespace kernel
{
    void init()
    {
        hardware::init();
        
        internal::m_context.old_time = 0U;
        internal::m_context.time = 0U;
        internal::m_context.schedule_lock = 0U;

        task::Id idle_task_handle;
        internal::task::create(
            internal::m_context.m_tasks,
            internal::task_routine,
            internal::idle_routine,
            task::Priority::Idle,
            &idle_task_handle
        );
        
        internal::scheduler::addTask(internal::m_context.m_scheduler, task::Priority::Idle, idle_task_handle);
        
        internal::m_context.m_current = idle_task_handle;
        internal::m_context.m_next = idle_task_handle;
    }
    
    void start()
    {
        internal::lockScheduler();
        internal::m_context.started = true;

        // Reschedule all tasks created before kernel::start
        internal::scheduler::findHighestPrioTask(internal::m_context.m_scheduler, internal::m_context.m_next);
        
        hardware::start();
        // Load first task
        hardware::syscall(hardware::SyscallId::LoadNextTask);
    }
}

namespace kernel::task
{
    bool create(
        kernel::task::Routine   a_routine,
        kernel::task::Priority  a_priority,
        kernel::task::Id *      a_handle,
        void *                  a_parameter,
        bool                    a_create_suspended
    )
    {

        const kernel::task::Priority currentTaskPrio = internal::task::priority::get(
            internal::m_context.m_tasks,
            internal::m_context.m_current
        );

        internal::lockScheduler();
        {
            kernel::task::Id id;

            bool task_created = kernel::internal::task::create(
                internal::m_context.m_tasks,
                internal::task_routine,
                a_routine, a_priority,
                &id,
                a_parameter,
                a_create_suspended
            );
        
            if (false == task_created)
            {
                internal::unlockScheduler();
                return false;
            }

            bool task_added = kernel::internal::scheduler::addTask(
                kernel::internal::m_context.m_scheduler,
                a_priority,
                id
            );

            if (false == task_added)
            {
                kernel::internal::task::destroy(internal::m_context.m_tasks, id);
                internal::unlockScheduler();
                return false;
            }

            if (a_handle)
            {
                *a_handle = id;
            }

            if (kernel::internal::m_context.started && (a_priority < currentTaskPrio))
            {
                kernel::internal::scheduler::findHighestPrioTask(
                    kernel::internal::m_context.m_scheduler,
                    kernel::internal::m_context.m_next
                );
            }
        }

        if (kernel::internal::m_context.started && (a_priority < currentTaskPrio))
        {
            hardware::syscall(hardware::SyscallId::ExecuteContextSwitch);
        }
        else
        {
            internal::unlockScheduler();
        }

        return true;
    }

    void terminate(kernel::task::Id a_id)
    {
        internal::lockScheduler();
        {
            kernel::task::Priority prio = internal::task::priority::get(
                internal::m_context.m_tasks,
                a_id
            );

            internal::scheduler::removeTask(internal::m_context.m_scheduler, prio, a_id);
            internal::task::destroy(internal::m_context.m_tasks, a_id);

            // reschedule in case task is killing itself
            if (kernel::internal::m_context.m_current.m_id == a_id.m_id)
            {
                kernel::internal::scheduler::findHighestPrioTask(
                    kernel::internal::m_context.m_scheduler,
                    kernel::internal::m_context.m_next
                );
            }
        }

        // If task is terminating itself, dont store the context.
        if (kernel::internal::m_context.m_current.m_id == a_id.m_id)
        {
            hardware::syscall(hardware::SyscallId::LoadNextTask);
        }
        else
        {
            internal::unlockScheduler();
        }
    }
}

namespace kernel::internal
{
    void loadNextTask()
    {
        loadContext(internal::m_context.m_next);
        internal::m_context.m_current = internal::m_context.m_next;
        
        internal::unlockScheduler();
    }

    void switchContext()
    {
        storeContext(m_context.m_current);
        loadContext(m_context.m_next);

        m_context.m_current = m_context.m_next;
        
        internal::unlockScheduler();
    }

    // TODO: all data must be in critical section
    //  ...or dont. So far everything is designed assuming kernel m_context is
    // accessed from handler mode only without nesting interrupts (int disabled during handler)
    // - this must be confirmed. Also possible to use designed priority to handle critical section
    bool tick()
    {
        bool execute_context_switch = false;
       
        // If lock is enabled, increment time, but delay scheduler.
        if (0 == m_context.schedule_lock)
        {
            // Calculate Round-Robin time stamp
            // TODO: this looks like shit - make something nicer.
            if (m_context.time - m_context.old_time > m_context.interval)
            {
                m_context.old_time = m_context.time;

                // Get current task priority.
                const kernel::task::Priority current  = internal::task::priority::get(internal::m_context.m_tasks, m_context.m_current);
            
                // Find next task in priority group.
                if(true == scheduler::findNextTask(m_context.m_scheduler, current, m_context.m_next))
                {
                    storeContext(m_context.m_current);
                    loadContext(m_context.m_next);

                    m_context.m_current = m_context.m_next;

                    execute_context_switch = true;
                }
            }
        }

        m_context.time++;
        
        return execute_context_switch;
    }
}
