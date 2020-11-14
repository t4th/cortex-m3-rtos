#include <kernel.hpp>
#include <hardware.hpp>
#include <scheduler.hpp>
#include <handle.hpp>

namespace kernel::internal
{
    typedef uint32_t Time_ms;

    constexpr Time_ms CONTEXT_SWITCH_INTERVAL_MS = 10U;
    
    struct Context
    {
        Time_ms old_time = 0U;  // Used to calculate round-robin context switch intervals.
        Time_ms time = 0U;      // Time in miliseconds elapsed since kernel started.
        Time_ms interval = CONTEXT_SWITCH_INTERVAL_MS; // Round-robin context switch intervals in miliseconds.
        
        kernel::internal::task::Id m_current; // Indicate currently running task ID.
        kernel::internal::task::Id m_next;    // Indicate next task ID.

        // TODO: make status register
        bool started = false; // Indicate if kernel is started by kernel::Start function.

        // Lock used to stop kernel from round-robin context switches.
        // 0 - context switch unlocked; !0 - context switch locked
        volatile uint32_t schedule_lock = 0U;

        // Data
        internal::task::Context      m_tasks;
        internal::scheduler::Context m_scheduler;
    } m_context;

    // Must only be called from handler mode (MSP stack) since it is modifying psp.
    void storeContext(kernel::internal::task::Id a_task)
    {
        const uint32_t sp = hardware::sp::get();
        internal::task::sp::set(m_context.m_tasks, a_task, sp);

        kernel::hardware::task::Context * current_task = internal::task::context::get(m_context.m_tasks, a_task);
        kernel::hardware::context::current::set(current_task);
    }

    // Must only be called from handler mode (MSP stack) since it is modifying psp.
    void loadContext(kernel::internal::task::Id a_task)
    {
        kernel::hardware::task::Context * next_task = internal::task::context::get(m_context.m_tasks, a_task);
        kernel::hardware::context::next::set(next_task);

        const uint32_t next_sp = kernel::internal::task::sp::get(m_context.m_tasks, a_task);
        kernel::hardware::sp::set(next_sp);
    }

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
        --internal::m_context.schedule_lock;
    }

    void terminateTask(kernel::internal::task::Id a_id)
    {
        internal::lockScheduler();
        {
            kernel::task::Priority prio = internal::task::priority::get(
                internal::m_context.m_tasks,
                a_id
            );

            internal::scheduler::removeTask(internal::m_context.m_scheduler, prio, a_id);
            internal::task::destroy(internal::m_context.m_tasks, a_id);

            // Reschedule in case task is killing itself.
            if (kernel::internal::m_context.m_current.m_id == a_id.m_id)
            {
                kernel::internal::scheduler::findHighestPrioTask(
                    kernel::internal::m_context.m_scheduler,
                    kernel::internal::m_context.m_next
                );

                hardware::syscall(hardware::SyscallId::LoadNextTask);
            }
            else
            {
                internal::unlockScheduler();
            }
        }
    }

    // User task routine wrapper used by kernel.
    void task_routine()
    {
        kernel::task::Routine routine = internal::task::routine::get(m_context.m_tasks, m_context.m_current );
        void * parameter = internal::task::parameter::get(m_context.m_tasks, m_context.m_current);

        routine(parameter); // Call the actual task routine.

        kernel::internal::terminateTask(m_context.m_current); // Cleanup task data.
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

        internal::task::Id idle_task_handle;

        // Idle task is always available as system task.
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

        internal::scheduler::findHighestPrioTask(internal::m_context.m_scheduler, internal::m_context.m_next);
        
        hardware::start();
        hardware::syscall(hardware::SyscallId::LoadNextTask);
    }
}

namespace kernel::task
{
    bool create(
        kernel::task::Routine   a_routine,
        kernel::task::Priority  a_priority,
        kernel::Handle *        a_handle,
        void *                  a_parameter,
        bool                    a_create_suspended
    )
    {
        internal::lockScheduler();
        {
            const kernel::task::Priority currentTaskPrio = internal::task::priority::get(
                internal::m_context.m_tasks,
                internal::m_context.m_current
            );

            kernel::internal::task::Id id;

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

            if (false == a_create_suspended)
            {
                bool task_added = kernel::internal::scheduler::addTask(
                    kernel::internal::m_context.m_scheduler,
                    a_priority,
                    id
                );

                if (false == task_added)
                {
                    kernel::internal::task::destroy( internal::m_context.m_tasks, id);
                    internal::unlockScheduler();
                    return false;
                }
            }

            if (a_handle)
            {
                *a_handle = internal::handle::create( internal::handle::ObjectType::Task, id.m_id);
            }

            if (kernel::internal::m_context.started && (a_priority < currentTaskPrio))
            {
                kernel::internal::scheduler::findHighestPrioTask(
                    kernel::internal::m_context.m_scheduler,
                    kernel::internal::m_context.m_next
                );
            }

            if (kernel::internal::m_context.started && (a_priority < currentTaskPrio))
            {
                hardware::syscall( hardware::SyscallId::ExecuteContextSwitch);
            }
            else
            {
                internal::unlockScheduler();
            }
        }

        return true;
    }

    kernel::Handle getCurrent()
    {
        kernel::Handle new_handle{};

        internal::lockScheduler();
        {
            new_handle = internal::handle::create(
                internal::handle::ObjectType::Task,
                internal::m_context.m_current.m_id
            );
        }
        internal::unlockScheduler();

        return new_handle;
    }

    void terminate(kernel::Handle a_id)
    {
        switch(internal::handle::getObjectType(a_id))
        {
        case internal::handle::ObjectType::Task:
        {
            internal::terminateTask({internal::handle::getIndex(a_id)});
            break;
        }
        default:
            break;
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

    bool tick()
    {
        bool execute_context_switch = false;
       
        // If lock is enabled, increment time, but delay scheduler.
        if (0 == m_context.schedule_lock)
        {
            // Calculate Round-Robin time stamp
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
