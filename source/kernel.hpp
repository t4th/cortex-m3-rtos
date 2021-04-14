#pragma once

#include <cstdint>
#include <atomic>

// Calling any of below functions from within hardware interrupt routine
// will cause undefined behaviour, unless specified otherwise.

// User API.
namespace kernel
{
    using TimeMs = uint32_t;

    // Abstract handle to system object.
    // Should only be used with kernel API.
    // Direct modification is UB.
    enum class Handle : uint32_t{};

    // Initialize kernel.
    // This is PRE-CONDITION to all other kernel functions!
    // If not called first, kernel behaviour is UNDEFINED.
    void init();

    // Start kernel. If no user tasks were created, jumps to IDLE task.
    void start();

    // Return time in miliseconds since kernel started.
    TimeMs getTime();

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
        High,
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
        kernel::Handle * const  a_handle = nullptr,
        void * const            a_parameter = nullptr,
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

    void sleep( TimeMs a_time);
}

// User API for controling software timers.
namespace kernel::timer
{
    bool create( kernel::Handle & a_handle, TimeMs a_interval);
    void destroy( kernel::Handle & a_handle);
    void start( kernel::Handle & a_handle);
    void stop( kernel::Handle & a_handle);
}

// User API for controling events.
// Events API can be used from within interrupt handler.
namespace kernel::event
{
    // If a_manual_reset is set to false, event will be reset when waitForObject
    // function completes. In other case, you have to manualy call reset.
    // ap_name parameter must be pointer to compile time available literal or UB.
    bool create(
        kernel::Handle &    a_handle,
        bool                a_manual_reset = false,
        const char * const  ap_name = nullptr
    );

    // ap_name parameter must point to compile time available literal or UB.
    bool open( kernel::Handle & a_handle, const char * const ap_name);
    void destroy( kernel::Handle & a_handle);
    void set( kernel::Handle & a_handle);
    void reset( kernel::Handle & a_handle);
}

// User API for controling software critical sections.
// Software critical section can be used to protect data shared between tasks.
// Note: It cannot be used from within interrupt handler!
//       For that purpose use kernel::hardware::critical_section
namespace kernel::critical_section
{
    // Modyfing this outside critical_section API is UB.
    struct Context
    {
        volatile std::atomic< uint32_t> m_lockCount;
        volatile uint32_t               m_spinLock;
        kernel::Handle                  m_event;
        kernel::Handle                  m_ownerTask; // Debug information.
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

// User API for synchronization functions.
namespace kernel::sync
{
    enum class WaitResult
    {
        ObjectSet,
        TimeoutOccurred,
        WaitFailed
    };

    // Can wait for system objects of type: Event, Timer, Queue.
    // NOTE: Destroying system objects used by this function will result in undefined behaviour.
    WaitResult waitForSingleObject(
        kernel::Handle &    a_handle,
        bool                a_wait_forver = true,
        TimeMs              a_timeout = 0U
    );

    // Wait for multiple system objects provided as an array of handles.
    // If a_wait_for_all is set, task will wait until ALL signals are signaled.
    // If a_wait_for_all is not set, optional argument a_signaled_item_index,
    // will return first signaled handle index.
    WaitResult waitForMultipleObjects(
        kernel::Handle * const  a_array_of_handles,
        uint32_t                a_number_of_elements,
        bool                    a_wait_for_all = true,
        bool                    a_wait_forver = true,
        TimeMs                  a_timeout = 0U,
        uint32_t * const        a_signaled_item_index = nullptr
    );
}

// Static queue API can be used from within interrupt handler.
namespace kernel::static_queue
{
    // Static memory buffer. Modyfing it outside queue API is UB.
    template < typename TType, size_t Size>
    struct Buffer
    {
        volatile TType m_data[ Size]; // Note: Not initialized on purpose.
    };
    
    // ap_name parameter must be pointer to compile time available literal or UB.
    bool create(
        kernel::Handle &      a_handle,
        size_t                a_data_max_size,
        size_t                a_data_type_size,
        volatile void * const ap_static_buffer,
        const char * const    ap_name = nullptr
    );
    
    // ap_name parameter must be pointer to compile time available literal or UB.
    bool open( kernel::Handle & a_handle, const char * const ap_name);
    void destroy( kernel::Handle & a_handle);
    bool send( kernel::Handle & a_handle, volatile void * const ap_data);
    bool receive( kernel::Handle & a_handle, volatile void * const ap_data);
    bool size( kernel::Handle & a_handle, size_t & a_size);
    bool isFull( kernel::Handle & a_handle, bool & a_is_full);
    bool isEmpty( kernel::Handle & a_handle, bool & a_is_empty);
    
    template < typename TType, size_t Size>
    inline bool create( kernel::Handle & a_handle, Buffer< TType, Size> & a_buffer, const char * ap_name = nullptr)
    {
        return create( a_handle, Size, sizeof( TType), &a_buffer.m_data, ap_name);
    }

    template < typename TType>
    inline bool send( kernel::Handle & a_handle, TType & a_data)
    {
        return send( a_handle, &a_data);
    }

    template < typename TType>
    inline bool receive( kernel::Handle & a_handle, TType & a_data)
    {
        return receive( a_handle, &a_data);
    }
}

namespace kernel::hardware
{
    namespace interrupt
    {
        namespace priority
        {
            // Priority groups.
            enum class Preemption
            {
                Critical,
                Kernel,
                User
            };

            // Sub-priorities of Priority groups.
            enum class Sub
            {
                High,
                Medium,
                Low
            };

            // Vendor interrupt ID must be set according to MCU vendor data sheet.
            // Using invalid interrupt ID value will result in Undefined Behaviour.
            void set(
                uint32_t    a_vendor_interrupt_id,
                Preemption  a_preemption_priority,
                Sub         a_sub_priority
            );
        }

        // Vendor interrupt ID must be set according to MCU vendor data sheet.
        // Using invalid interrupt ID value will result in Undefined Behaviour.
        void enable( uint32_t a_vendor_interrupt_id);

        // Stop core until interrupt occur.
        void wait();
    }

    // Hardware level critical section used for protecting shared data between interrupts.
    namespace critical_section
    {
        struct Context
        {
            volatile uint32_t m_local_data{};
        };

        void enter( 
            Context &                       a_context,
            interrupt::priority::Preemption a_preemption_priority
        );

        void leave( Context & a_context);
    }

    // Helper RAII style hardware critical section.
    class CriticalSection
    {
    public:
        CriticalSection() = delete;
        inline CriticalSection( const interrupt::priority::Preemption & a_preemption_priority)
        {
           critical_section::enter( m_context, a_preemption_priority);
        }
        inline ~CriticalSection()
        {
            critical_section::leave( m_context);
        }
    private:
        critical_section::Context m_context{};
    };

    namespace debug
    {
        void putChar( char c);
        void print( const char * s);
        void setBreakpoint();
    }
}
