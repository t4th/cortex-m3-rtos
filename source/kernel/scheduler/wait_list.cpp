#include <wait_list.hpp>

namespace
{
    inline bool find(
        kernel::internal::common::MemoryBuffer<
        kernel::internal::scheduler::wait_list::WaitItem,
        kernel::internal::task::MAX_NUMBER
    > &                              a_list,
        kernel::internal::task::Id & a_key,
        uint32_t &                   a_index
        )
    {
        for (uint32_t i = 0U; i < kernel::internal::task::MAX_NUMBER; ++i)
        {
            if (true == a_list.isAllocated(i))
            {
                if (a_list.at(i).m_id == a_key)
                {
                    a_index = i;
                    return true;
                }
            }
        }
        return false;
    }

    // Search for task_id. If found return already existing Id,
    // otherwise allocate new item and return its Id.
    inline bool allocate(
        kernel::internal::common::MemoryBuffer<
        kernel::internal::scheduler::wait_list::WaitItem,
        kernel::internal::task::MAX_NUMBER
        > &                             a_list,
        kernel::internal::task::Id &    a_task_id,
        uint32_t &                      a_index
    )
    {
        bool is_item_found;

        is_item_found = find(a_list, a_task_id, a_index);

        if (false == is_item_found)
        {
            bool item_added = a_list.allocate(a_index);

            if (true == item_added)
            {
                a_list.at(a_index).m_id = a_task_id;
            }
            else
            {
                return false;
            }
        }
        return true;
    }
}

namespace kernel::internal::scheduler::wait_list
{
    bool addTaskSleep(
        Context &   a_context,
        task::Id    a_task_id,
        Time_ms     a_interval
    )
    {
        uint32_t    item_index;
        bool        allocated;

        allocated = allocate(a_context.m_list, a_task_id, item_index);
        
        if (false == allocated)
        {
            return false;
        }

        volatile wait::Conditions & conditions =
            a_context.m_list.at(item_index).m_conditions;

        wait::initSleep(conditions, a_interval);

        return true;
    }

    bool addTaskWaitObj(
        Context &           a_context,
        task::Id            a_task_id,
        kernel::Handle &    a_waitingSignal,
        bool                a_wait_forver,
        Time_ms             a_timeout
    )
    {
        uint32_t    item_index;
        bool        allocated;

        allocated = allocate(a_context.m_list, a_task_id, item_index);

        if (false == allocated)
        {
            return false;
        }

        volatile wait::Conditions & conditions =
            a_context.m_list.at(item_index).m_conditions;

        wait::initWaitForObj(
            conditions,
            a_waitingSignal,
            a_waitingSignal,
            a_timeout
        );

        return true;
    }

    void checkConditions(
        Context &                       a_context,
        internal::scheduler::Context &  a_scheduler_context,
        internal::task::Context &       a_task_context,
        internal::timer::Context &      a_timer_context,
        internal::event::Context &      a_event_context,
        void condition_fulfilled(
            internal::scheduler::Context &  a_scheduler_context,
            internal::task::Context &       a_task_context,
            volatile task::Id &             a_task_id
        )
    )
    {
        for (uint32_t i = 0U; i < kernel::internal::task::MAX_NUMBER; ++i)
        {
            if (true == a_context.m_list.isAllocated(i))
            {
                kernel::sync::WaitResult a_wait_result;

                bool is_condition_fulfilled =
                    wait::check(
                        a_context.m_list.at(i).m_conditions,
                        a_timer_context,
                        a_event_context,
                        a_wait_result
                    );

                if (true == is_condition_fulfilled)
                {
                    condition_fulfilled(
                        a_scheduler_context,
                        a_task_context,
                        a_context.m_list.at(i).m_id
                    );
                    a_context.m_list.free(i); // Remove condition data.
                }
            }
        }
    }

    void removeTask(
        Context & a_context,
        task::Id a_task_id
    )
    {
        uint32_t found_index;
        bool item_found;

        item_found = find(a_context.m_list, a_task_id, found_index);

        if (item_found)
        {
            a_context.m_list.free(found_index);
        }
    }
}
