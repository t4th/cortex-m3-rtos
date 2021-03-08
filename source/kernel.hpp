#pragma once

#include <cstdint>
#include <atomic>

// Calling any of below functions from within hardware interrupt routine
// will cause undefined behaviour, unless specified otherwise.

// User API.
namespace kernel
{
    typedef uint32_t Time_ms;

    // Abstract handle to system object.
    // Should only be used with kernel API.
    // Direct modification is UB.
    typedef volatile void * Handle;

    // Initialize kernel.
    // This is PRE-CONDITION to all other kernel functions!
    // If not called first, kernel behaviour is UNDEFINED.
    void init();

    // Start kernel. If no user tasks were created, jumps to IDLE task.
    void start();

    // Return time in miliseconds since kernel started.
    Time_ms getTime();

    // Return core frequency in Hz.
    uint32_t getCoreFrequencyHz();
}

// User API for controling tasks.
namespace kernel::task
{
    typedef void( *Routine)( void * a_parameter);

    // Task priority. Idle task MUST always be available and MUST
    // always be last in Priority enum. Otherwise UB.
    enum class Priority
    {
        High = 0U,
        Medium,
        Low,
        Idle
    };

    enum class State
    {
        Suspended,
        Waiting,
        Ready,
        Running
    };

    // Create new task. Can be created statically (before calling kernel::start()),
    // and in run-time, by other tasks.
    bool create(
        kernel::task::Routine   a_routine,
        kernel::task::Priority  a_priority = kernel::task::Priority::Low,
        kernel::Handle *        a_handle = nullptr,
        void *                  a_parameter = nullptr,
        bool                    a_create_suspended = false
    );

    // Return Handle to currently running task.
    kernel::Handle getCurrent();

    // Brute force terminate task. Can cause UB if task is system object owner, or
    // task was inside critical section.
    void terminate( kernel::Handle & a_handle);

    // Suspend task execution. Task can suspend itself.
    void suspend( kernel::Handle & a_handle);

    // Works only with Suspended tasks. Has no effect on Ready or Waiting tasks.
    void resume( kernel::Handle & a_handle);

    void sleep( Time_ms a_time);
}

// User API for controling software timers.
namespace kernel::timer
{
    bool create(
        kernel::Handle &    a_handle,
        Time_ms             a_interval
    );
    void destroy( kernel::Handle & a_handle);
    void start( kernel::Handle & a_handle);
    void stop( kernel::Handle & a_handle);
}

// User API for controling events.
namespace kernel::event
{
    // If a_manual_reset is set to false, event will be reset when waitForObject
    // function completes. In other case, you have to manualy call reset.
    bool create( kernel::Handle & a_handle, bool a_manual_reset = false);
    void destroy( kernel::Handle & a_handle);
    void set( kernel::Handle & a_handle);

    // Must only be used from within Interrupt Routine.
    void setFromInterrupt( kernel::Handle & a_handle);

    void reset( kernel::Handle & a_handle);
}

// User API for controling critical section.
namespace kernel::critical_section
{
    // Modyfing this outside critical_section API is UB.
    struct Context
    {
        volatile std::atomic< uint32_t> m_lockCount;
        volatile uint32_t m_spinLock;
        kernel::Handle m_event;
        kernel::Handle m_ownerTask; // debug information
    };

    // Spinlock argument define number of ticks used to check critical section
    // condition. Experimenting with its value can improve performance, because
    // it can reduce number of context switches.
    bool init( Context & a_context, uint32_t a_spinLock = 100U);
    void deinit( Context & a_context);

    // Calling enter/leave without calling init first will cause UB.
    void enter( Context & a_context);
    void leave( Context & a_context);
}

namespace kernel::sync
{
    enum class WaitResult
    {
        ObjectSet,
        TimeoutOccurred,
        WaitFailed,         // Return if any internal error occurred.
        InvalidHandle
    };

    // Can wait for system objects of type: Event, Timer.
    // Wait is successful when event object is in SET state and Timer
    // is in FINISHED state. If used with invalid handle, error is returned.

    // The moment function is called, and provided signals are not signaled,
    // task will enter Waiting state until they do, or optional timeout is
    // reached.

    // If a_wait_forver is true, then a_timeout is not used.

    // Function return ObjectSet if signal condittion is met.

    // NOTE: Destroying system objects used by this function will result
    //       in undefined behaviour
    WaitResult waitForSingleObject(
        kernel::Handle &    a_handle,
        bool                a_wait_forver = true,
        Time_ms             a_timeout = 0U
    );

    // Wait for multiple system objects provided as an array of handles.

    // If a_wait_for_all is set, task will wait until ALL signals are signaled.

    // If a_wait_for_all is not set, optional argument a_signaled_item_index,
    // will return first signaled handle index.
    WaitResult waitForMultipleObjects(
        kernel::Handle *    a_array_of_handles,
        uint32_t            a_number_of_elements,
        bool                a_wait_for_all = true,
        bool                a_wait_forver = true,
        Time_ms             a_timeout = 0U,
        uint32_t *          a_signaled_item_index = nullptr
    );
}

namespace kernel::static_queue
{
    template < typename TType, size_t Size>
    struct Buffer
    {
        TType m_data[ Size];
    };

    bool create(
        kernel::Handle &    a_handle,
        size_t              a_data_size,
        void * const        ap_data
    );

    template < typename TType, size_t Size>
    bool create( kernel::Handle & a_handle, Buffer< TType, Size> & a_buffer)
    {
        return create( a_handle, sizeof( TType), &a_buffer.m_data);
    }

    template < typename TType>
    bool send( kernel::Handle & a_handle, TType & a_data)
    {
        return false;
    }

    template < typename TType>
    bool receive( kernel::Handle & a_handle, TType & a_data)
    {
        return false;
    }

    template < typename TType>
    bool sendFromInterrupt( kernel::Handle & a_handle, TType & a_data)
    {
        return false;
    }

    template < typename TType>
    bool receiveFromInterrupt( kernel::Handle & a_handle, TType & a_data)
    {
        return false;
    }

    template < typename TType>
    bool at( kernel::Handle & a_handle, size_t a_index)
    {
        return false;
    }

    size_t size( kernel::Handle & a_handle);
}

namespace kernel::hardware
{
    namespace interrupt
    {
        namespace priority
        {
            // Priority groups. Smaller value is higher priority.
            enum class Preemption
            {
                Critical = 1U,
                Kernel = 2U,
                User = 3U
            };

            // Sub-priorities of Priority groups.
            // Smaller value is higher priority within group.
            enum class Sub
            {
                High = 0U,
                Medium = 1U,
                Low = 2U
            };

            // a_vendor_interrupt_id must be set according to vendor data sheet.
            // Vendor interrupts always starts from 0.
            // Using invalid interrupt ID value will result in Undefined Behaviour.
            void set(
                uint32_t    a_vendor_interrupt_id,
                Preemption  a_preemption_priority,
                Sub         a_sub_priority
            );
        }

        // a_vendor_interrupt_id must be set according to vendor data sheet.
        // Vendor interrupts always starts from 0.
        // Using invalid interrupt ID value will result in Undefined Behaviour.
        void enable( uint32_t a_vendor_interrupt_id);

        // Stop core until interrupt occur.
        void wait();
    }

    // Hardware level critical section used for protecting data accessed by different interrupts.
    namespace critical_section
    {
        struct Context
        {
            uint32_t m_local_data{};
        };

        void enter(
            volatile Context &                  a_context,
            interrupt::priority::Preemption     a_preemption_priority
        );

        void leave( volatile Context & a_context);
    }

    namespace debug
    {
        void putChar( char c);
        void print( const char * s);
        void setBreakpoint();
    }
}

