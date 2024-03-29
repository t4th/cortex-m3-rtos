#pragma once

#include "config/config.hpp"

#include "timer/timer.hpp"
#include "event/event.hpp"
#include "queue/queue.hpp"
#include "handle/handle.hpp"

// This is data structure holding task wait conditions.
// Before, I kept this data attached to each internal::task,
// and I might consider that again in the future. Why I did
// split it from internal::task in the first place, was that
// I didn't want to complicate POD-style internal::task:Context
// and internal::task API.
namespace kernel::internal::scheduler::wait
{
    enum class Type
    {
        Sleep,
        WaitForObj
    };

    struct Conditions
    {
        Type        m_type;

        Handle      m_waitSignals[ max_input_signals];
        uint32_t    m_numberOfSignals;

        // Indicate if all m_waitSignals signals must be set to fulfil wake up condition.
        bool        m_waitForAllSignals;

        // Indicate if there is timeout when waiting for m_waitSignals signals.
        bool        m_waitForver;

        // TODO: consider using internal::timer for wait states.
        TimeMs      m_interval;
        TimeMs      m_start;
    };

    inline void initSleep(
        volatile Conditions &   a_conditions_context,
        TimeMs &                a_interval,
        TimeMs &                a_current
    )
    {
        a_conditions_context.m_type = Type::Sleep;
        a_conditions_context.m_interval = a_interval;
        a_conditions_context.m_start = a_current;
    }

    inline bool initWaitForObj(
        volatile Conditions &   a_conditions_context,
        kernel::Handle *        a_wait_signals,
        uint32_t                a_number_of_signals,
        bool &                  a_wait_for_all_signals,
        bool &                  a_wait_forver,
        TimeMs &                a_timeout,
        TimeMs &                a_current
    )
    {
        assert( a_wait_signals);
        assert( a_number_of_signals > 0U);

        if ( a_number_of_signals < max_input_signals)
        {
            a_conditions_context.m_numberOfSignals = a_number_of_signals;
        }
        else
        {
            return false;
        }

        for ( uint32_t i = 0U; i < a_conditions_context.m_numberOfSignals; ++i)
        {
            a_conditions_context.m_waitSignals[ i] = a_wait_signals[ i];
        }

        a_conditions_context.m_waitForAllSignals = a_wait_for_all_signals;
        a_conditions_context.m_type = Type::WaitForObj;
        a_conditions_context.m_waitForver = a_wait_forver;
        a_conditions_context.m_interval = a_timeout;
        a_conditions_context.m_start = a_current;

        return true;
    }

    // Test and UPDATE wait signals depending on provided context.
    inline bool testWaitSignals(
        internal::timer::Context &  a_timer_context,
        internal::event::Context &  a_event_context,
        internal::queue::Context &  a_queue_context,
        kernel::sync::WaitResult &  a_result,
        volatile kernel::Handle *   a_wait_signals,
        uint32_t                    a_number_of_signals,
        volatile bool &             a_wait_for_all_signals,
        uint32_t &                  a_signaled_item_index
    )
    {
        bool condition_fulfilled = true;

        // TODO: seperate late-decision bools from loop.
        //       Especially a_wait_for_all_signals.
        for ( uint32_t i = 0U; i < a_number_of_signals; ++i)
        {
            const bool valid_handle = handle::testCondition(
                a_timer_context,
                a_event_context,
                a_queue_context,
                a_wait_signals[ i],
                condition_fulfilled
            );

            if ( false == valid_handle)
            {
                // Not supported handle type.
                a_result = kernel::sync::WaitResult::WaitFailed;
                return false;
            }

            condition_fulfilled &= condition_fulfilled;

            // When not waiting for all signals to be set,
            // return on first condition fulfilled.
            if ( false == a_wait_for_all_signals)
            {
                if ( true == condition_fulfilled)
                {
                    handle::resetState( a_timer_context, a_event_context, a_wait_signals[ i]);
                    
                    a_signaled_item_index = i;
                    a_result = kernel::sync::WaitResult::ObjectSet;
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                // When waiting for all signals to be set,
                // return on first unfulfilled condition.
                if ( false == condition_fulfilled)
                {
                    return false;
                }
                else
                {
                    // Continue checking other events.
                }
            }
        }

        // Check if all provided signals are set.
        if ( true == a_wait_for_all_signals)
        {
            if ( true == condition_fulfilled)
            {
                // Reset all system objects pointed by a_wait_signals.
                for ( uint32_t i = 0U; i < a_number_of_signals; ++i)
                {
                    handle::resetState( a_timer_context, a_event_context, a_wait_signals[ i]);
                }
                a_result = kernel::sync::WaitResult::ObjectSet;
                return true;
            }
        }

        return false;
    }

    inline bool check(
        volatile Conditions &       a_conditions_context,
        internal::timer::Context &  a_timer_context,
        internal::event::Context &  a_event_context,
        internal::queue::Context &  a_queue_context,
        kernel::sync::WaitResult &  a_result,
        TimeMs &                    a_current,
        uint32_t &                  a_signaled_item_index
        )
    {
        switch( a_conditions_context.m_type)
        {
        case Type::Sleep:
        {
            if ( a_current - a_conditions_context.m_start > a_conditions_context.m_interval)
            {
                a_result = kernel::sync::WaitResult::ObjectSet;
                return true;
            }
        }
        break;
        case Type::WaitForObj:
        {
            a_signaled_item_index = 0U;

            if ( false == a_conditions_context.m_waitForver)
            {
                // Check timeout before wait signals.
                if ( a_current - a_conditions_context.m_start > a_conditions_context.m_interval)
                {
                    a_result = kernel::sync::WaitResult::TimeoutOccurred;
                    return true;
                }
            }

            bool condition_fulfilled = testWaitSignals(
                a_timer_context,
                a_event_context,
                a_queue_context,
                a_result,
                a_conditions_context.m_waitSignals,
                a_conditions_context.m_numberOfSignals,
                a_conditions_context.m_waitForAllSignals,
                a_signaled_item_index
            );

            return condition_fulfilled;
        }
        break;
        default:
        break;
        }

        return false;
    }
}
