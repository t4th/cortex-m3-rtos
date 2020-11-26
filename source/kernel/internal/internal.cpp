#include <internal.hpp>

namespace kernel::internal
{
    Context m_context;

    void storeContext(kernel::internal::task::Id a_task);
    void loadContext(kernel::internal::task::Id a_task);
}

namespace kernel::internal
{
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
        kernel::internal::task::state::set(
            internal::m_context.m_tasks,
            a_task,
            kernel::task::State::Running
        );

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
        // TODO: analyze DSB and DMB here
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

                if (kernel::internal::m_context.started)
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
                    kernel::internal::task::state::set(
                        internal::m_context.m_tasks,
                        internal::m_context.m_current,
                        kernel::task::State::Ready
                    );

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
