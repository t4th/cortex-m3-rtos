#pragma once

#include <timer.hpp>
#include <event.hpp>
#include <handle.hpp>

namespace kernel::internal::scheduler::wait
{
    constexpr uint32_t MAX_INPUT_SIGNALS = 8U;

    enum class Type
    {
        Sleep,
        WaitForObj
    };

    struct Conditions
    {
        Type        m_type;

        Handle      m_waitSignals[MAX_INPUT_SIGNALS];
        uint32_t    m_numberOfSignals;

        // Indicate if all m_waitSignals signals must be set to fulfil wake up condition.
        bool        m_waitForAllSignals;

        // Indicate if there is timeout when waiting for m_waitSignals signals.
        bool        m_waitForver;

        Time_ms     m_interval;
        Time_ms     m_start;
    };

    inline void initSleep(
        volatile Conditions &   a_conditions,
        Time_ms &               a_interval,
        Time_ms &               a_current
    )
    {
        a_conditions.m_type = Type::Sleep;
        a_conditions.m_interval = a_interval;
        a_conditions.m_start = a_current;
    }

    inline bool initWaitForObj(
        volatile Conditions &   a_conditions,
        kernel::Handle *        a_wait_signals,
        uint32_t                a_number_of_signals,
        bool &                  a_wait_for_all_signals,
        bool &                  a_wait_forver,
        Time_ms &               a_timeout,
        Time_ms &               a_current
    )
    {
        assert(a_wait_signals);
        assert(a_number_of_signals > 0U);

        if (nullptr == a_wait_signals)
        {
            return false;
        }

        if (a_number_of_signals < MAX_INPUT_SIGNALS)
        {
            a_conditions.m_numberOfSignals = a_number_of_signals;
        }
        else
        {
            return false;
        }

        for (uint32_t i = 0U; i < a_conditions.m_numberOfSignals; ++i)
        {
            a_conditions.m_waitSignals[i] = a_wait_signals[i];
        }

        a_conditions.m_waitForAllSignals = a_wait_for_all_signals;
        a_conditions.m_type = Type::WaitForObj;
        a_conditions.m_waitForver = a_wait_forver;
        a_conditions.m_interval = a_timeout;
        a_conditions.m_start = a_current;

        return true;
    }

    // TODO: this is code dublicate of kernel::sync::testHandleCondition.
    inline bool testSignalCondition(
        internal::timer::Context &  a_timer_context,
        internal::event::Context &  a_event_context,
        volatile kernel::Handle &   a_handle,
        bool &                      a_condition_fulfilled
    )
    {
        const auto objectType = internal::handle::getObjectType(a_handle);

        a_condition_fulfilled = false;

        switch (objectType)
        {
        case internal::handle::ObjectType::Event:
        {
            auto event_id = internal::handle::getId<internal::event::Id>(a_handle);
            auto evState = internal::event::getState(a_event_context, event_id);
            if (internal::event::State::Set == evState)
            {
                a_condition_fulfilled = true;
            }
            break;
        }
        case internal::handle::ObjectType::Timer:
        {
            auto timer_id = internal::handle::getId<internal::timer::Id>(a_handle);
            auto timerState = internal::timer::getState(a_timer_context, timer_id);
            if (internal::timer::State::Finished == timerState)
            {
                a_condition_fulfilled = true;
            }
            break;
        }
        default:
        {
            return false; // Handle is pointing to unsupported system object type
        }
        };

        return true; // Handle is pointing to supported system object type
    }

    inline bool check(
        volatile Conditions &       a_conditions,
        internal::timer::Context &  a_timer_context,
        internal::event::Context &  a_event_context,
        kernel::sync::WaitResult &  a_result,
        Time_ms &                   a_current,
        uint32_t &                  a_signaled_item_index
        )
    {
        switch(a_conditions.m_type)
        {
        case Type::Sleep:
        {
            if (a_current - a_conditions.m_start > a_conditions.m_interval)
            {
                return true;
            }
        }
        break;
        case Type::WaitForObj:
        {
            bool condition_fulfilled = true;

            a_signaled_item_index = 0U;

            if (false == a_conditions.m_waitForver)
            {
                if (a_current - a_conditions.m_start > a_conditions.m_interval)
                {
                    a_result = kernel::sync::WaitResult::TimeoutOccurred;
                    return true;
                }
            }

            // TODO: seperate later-decision bools from loop.
            for (uint32_t i = 0; i < a_conditions.m_numberOfSignals; ++i)
            {
                bool valid_handle = testSignalCondition(
                    a_timer_context,
                    a_event_context,
                    a_conditions.m_waitSignals[i],
                    condition_fulfilled
                );

                if (false == valid_handle)
                {
                    a_result = kernel::sync::WaitResult::InvalidHandle;
                    return true;
                }

                condition_fulfilled &= condition_fulfilled;

                // When not waiting for all signals to be set,
                // return on first condition fulfilled.
                if (false == a_conditions.m_waitForAllSignals)
                {
                    if (true == condition_fulfilled)
                    {
                        a_signaled_item_index = i;
                        a_result = kernel::sync::WaitResult::ObjectSet;
                        return true;
                    }
                }
            }

            if (true == a_conditions.m_waitForAllSignals)
            {
                if (true == condition_fulfilled)
                {
                    a_result = kernel::sync::WaitResult::ObjectSet;
                    return true;
                }
            }
        }
        break;
        default:
        break;
        }

        return false;
    }
}
