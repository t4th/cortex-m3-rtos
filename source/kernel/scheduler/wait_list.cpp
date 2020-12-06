#include <wait_list.hpp>

namespace kernel::internal::scheduler::wait_list
{
    bool addTask( Context & a_context, task::Id a_task_id)
    {
        uint32_t found_index;
        bool item_found = a_context.m_wait_list.find(
            a_task_id,
            found_index,
            [] (internal::task::Id & a_left, volatile internal::task::Id & a_right) -> bool
            {
                return a_left == a_right;
            });

        if (item_found)
        {
            return false;
        }

        uint32_t new_node_idx;
        bool task_added =a_context.m_wait_list.add(a_task_id, new_node_idx);

        if (false == task_added)
        {
            return false;
        }

        return true;
    }

    void removeTask( Context & a_context, task::Id a_task_id)
    {
        uint32_t found_index;
        bool item_found = a_context.m_wait_list.find(
            a_task_id,
            found_index,
            [] (internal::task::Id & a_left, volatile internal::task::Id & a_right) -> bool
            {
                return a_left == a_right;
            });

        if (item_found)
        {
            a_context.m_wait_list.remove(found_index);
        }
    }

    void iterate(
        wait_list::Context &        a_wait_list,
        ready_list::Context &       a_ready_list,
        internal::task::Context &   a_task_context,
        internal::timer::Context &  a_timer_context,
        internal::event::Context &  a_event_context,
        void        found(
            wait_list::Context &,
            ready_list::Context &,
            internal::task::Context &,
            internal::timer::Context &,
            internal::event::Context &,
            task::Id &
        )
    )
    {
        const uint32_t count = a_wait_list.m_wait_list.count();
        uint32_t item_index = a_wait_list.m_wait_list.firstIndex();

        for (uint32_t i = 0U; i < count; ++i)
        {
            internal::task::Id found_id = a_wait_list.m_wait_list.at(item_index);
            item_index = a_wait_list.m_wait_list.nextIndex(item_index);

            found(
                a_wait_list,
                a_ready_list,
                a_task_context,
                a_timer_context,
                a_event_context,
                found_id);
        }
    }
}
