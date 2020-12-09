#pragma once

#include <timer.hpp>
#include <event.hpp>
#include <handle.hpp>

namespace kernel::internal::scheduler::wait
{
    constexpr uint32_t MAX_INPUT_SIGNALS = 8U;
    constexpr uint32_t MAX_OUTPUT_SIGNALS = 8U;

    enum class Type
    {
        Sleep,
        WaitForObj
    };

    struct Conditions
    {
        Type m_type;

        // TODO: make these as lists to reduce iterations
        internal::common::MemoryBuffer<Handle, MAX_INPUT_SIGNALS>   m_waitSignals{};

        // TODO: implement two-way signal tracking
        // internal::common::MemoryBuffer<Handle, MAX_OUTPUT_SIGNALS>  m_outputSignals{};

        bool    m_waitForver;
        Time_ms m_interval;
        Time_ms m_start;
    };

    inline void initSleep(
        volatile Conditions &   a_conditions,
        Time_ms &               a_interval,
        Time_ms &               a_current
    )
    {
        a_conditions.m_waitSignals.freeAll();

        a_conditions.m_type = Type::Sleep;
        a_conditions.m_interval = a_interval;
        a_conditions.m_start = a_current;
    }

    inline bool initWaitForObj(
        volatile Conditions &   a_conditions,
        kernel::Handle &        a_waitingSignal,
        bool &                  a_wait_forver,
        Time_ms &               a_timeout,
        Time_ms &               a_current
    )
    {
        a_conditions.m_waitSignals.freeAll();

        uint32_t new_item;
        if (false == a_conditions.m_waitSignals.allocate(new_item))
        {
            return false;
        }

        a_conditions.m_waitSignals.at(new_item) = a_waitingSignal;
        
        a_conditions.m_type = Type::WaitForObj;
        a_conditions.m_waitForver = a_wait_forver;
        a_conditions.m_interval = a_timeout;
        a_conditions.m_start = a_current;

        return true;
    }

    inline bool check(
        volatile Conditions &       a_conditions,
        internal::timer::Context &  a_timer_context,
        internal::event::Context &  a_event_context,
        kernel::sync::WaitResult &  a_result,
        Time_ms &                   a_current
        )
    {
        bool condition_fulfilled = false;

        switch(a_conditions.m_type)
        {
        case Type::Sleep:
        {
            if (a_current - a_conditions.m_start > a_conditions.m_interval)
            {
                condition_fulfilled = true;
            }
        }
        break;
        case Type::WaitForObj:
        {
            if (false == a_conditions.m_waitForver)
            {
                if (a_current - a_conditions.m_start > a_conditions.m_interval)
                {
                    a_result = kernel::sync::WaitResult::TimeoutOccurred;
                    condition_fulfilled = true;
                    break;
                }
            }

            for (uint32_t i = 0; i < MAX_INPUT_SIGNALS; ++i)
            {
                if (false == a_conditions.m_waitSignals.isAllocated(i))
                {
                    break;
                }

                volatile kernel::Handle & objHandle = a_conditions.m_waitSignals.at(i);
                internal::handle::ObjectType objType =
                    internal::handle::getObjectType(objHandle);

                switch (objType)
                {
                case internal::handle::ObjectType::Event:
                {
                    auto event_id = internal::handle::getId<internal::event::Id>(objHandle);
                    event::State evState = event::getState(a_event_context, event_id);
                    if (event::State::Set == evState)
                    {
                        a_result = kernel::sync::WaitResult::ObjectSet;
                        condition_fulfilled = true;
                    }
                    break;
                }
                case internal::handle::ObjectType::Timer:
                {
                    auto timer_id = internal::handle::getId<internal::timer::Id>(objHandle);
                    timer::State timerState = timer::getState(a_timer_context, timer_id);

                    if (timer::State::Finished == timerState)
                    {
                        a_result = kernel::sync::WaitResult::ObjectSet;
                        condition_fulfilled = true;
                    }
                    break;
                }
                default:
                    break;
                };
            }
        }
        break;
        default:
        break;
        }

        return condition_fulfilled;
    }
}
