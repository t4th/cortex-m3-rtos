#include <internal.hpp>

namespace kernel::internal
{
    Context m_context;

    void storeContext(kernel::internal::task::Id a_task);
    void loadContext(kernel::internal::task::Id a_task);
}

namespace kernel::internal
{
    void init()
    {
        internal::task::Id idle_task_handle;

        // Idle task is always available as system task.
        // todo: check if kernel::task::create can be used instead of internal::task::create
        internal::task::create(
            m_context.m_tasks,
            internal::task_routine,
            internal::idle_routine,
            kernel::task::Priority::Idle,
            &idle_task_handle
        );

        internal::scheduler::addReadyTask(
            m_context.m_scheduler,
            m_context.m_tasks,
            kernel::task::Priority::Idle,
            idle_task_handle
        );
    }

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
        ++m_context.schedule_lock;
    }

    void unlockScheduler()
    {
        // TODO: analyze DSB and DMB here
        // TODO: thinker about removing it.
        --m_context.schedule_lock;
    }

    void terminateTask(kernel::internal::task::Id a_id)
    {
        internal::lockScheduler();
        {
            // Reschedule in case task is killing itself.
            internal::task::Id currentTask;
            internal::scheduler::getCurrentTaskId(m_context.m_scheduler, currentTask);

            internal::scheduler::removeTask(
                m_context.m_scheduler,
                m_context.m_tasks,
                a_id
            );

            internal::task::destroy(m_context.m_tasks, a_id);

            if (currentTask.m_id == a_id.m_id)
            {
                if (m_context.started)
                {
                    hardware::syscall(hardware::SyscallId::LoadNextTask);
                }
                else
                {
                    internal::unlockScheduler();
                }
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
        kernel::task::Routine routine{};
        void * parameter{};
        internal::task::Id currentTask;

        internal::lockScheduler();
        {
            internal::scheduler::getCurrentTaskId(m_context.m_scheduler, currentTask);
            routine = internal::task::routine::get(m_context.m_tasks, currentTask);
            parameter = internal::task::parameter::get(m_context.m_tasks, currentTask);
        }
        internal::unlockScheduler();

        routine(parameter); // Call the actual task routine.

        kernel::internal::terminateTask(currentTask); // Cleanup task data.
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

namespace kernel::internal
{
    void loadNextTask()
    {
        internal::task::Id nextTask;

        bool task_available = scheduler::getCurrentTask(
            m_context.m_scheduler,
            m_context.m_tasks,
            nextTask
        );

        assert(true == task_available);

        loadContext(nextTask);

        internal::unlockScheduler();
    }

    void switchContext()
    {
        internal::task::Id currentTask;
        internal::task::Id nextTask;

        scheduler::getCurrentTaskId(
            m_context.m_scheduler,
            currentTask
        );

        bool task_available = scheduler::getCurrentTask(
            m_context.m_scheduler,
            m_context.m_tasks,
            nextTask
        );

        assert(true == task_available);

        storeContext(currentTask);
        loadContext(nextTask);

        internal::unlockScheduler();
    }

    bool tick()
    {
        bool execute_context_switch = false;

        timer::tick( m_context.m_timers);
        scheduler::checkWaitConditions(
            m_context.m_scheduler,
            m_context.m_tasks,
            m_context.m_timers,
            m_context.m_events
        );

        // If lock is enabled, increment time, but delay scheduler.
        if (0U == m_context.schedule_lock)
        {
            // Calculate Round-Robin time stamp
            bool interval_elapsed = system_timer::isIntervalElapsed(
                m_context.m_systemTimer
            );

            if (interval_elapsed)
            {
                internal::task::Id currentTask;
                scheduler::getCurrentTaskId(
                    m_context.m_scheduler,
                    currentTask
                );
                
                // Find next task in priority group.
                internal::task::Id nextTask;
                bool task_found = scheduler::getNextTask(
                    m_context.m_scheduler,
                    m_context.m_tasks,
                    nextTask
                );

                if(task_found)
                {
                    storeContext(currentTask);
                    loadContext(nextTask);

                    execute_context_switch = true;
                }
            }
        }

        system_timer::increment( m_context.m_systemTimer);

        return execute_context_switch;
    }
}
