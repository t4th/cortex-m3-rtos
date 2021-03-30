// Contains example used to verify if tasks are switching correctly.
// Test: Create two tasks trying to access shared data with and without critical section.

// Expected: Without critical section enabled, artifacts should be visible in output window.

#include <kernel.hpp>

#include <cctype>
#include <cstring>

// Example options:
// true - enable use of critical section
// false - disable use of critical section
constexpr bool use_critical_section = true;

constexpr size_t MAX{ 100U};

struct SharedData
{
    kernel::critical_section::Context cs_context;
    char        text[ MAX];
    size_t      size;
};

void changeCaseSize( SharedData & a_shared_data)
{
    for ( size_t i = 0U; i < a_shared_data.size; ++i)
    {
        if ( isupper( a_shared_data.text[ i]))
        {
            a_shared_data.text[ i] = tolower( a_shared_data.text[ i]);
        }
        else
        {
            a_shared_data.text[ i] = toupper( a_shared_data.text[ i]);
        }
    }
}

void worker_task( void * a_parameter);

int main()
{
    SharedData shared_data;

    strcpy( shared_data.text, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n");
    shared_data.size = strlen( shared_data.text);

    kernel::init();

    if constexpr( use_critical_section)
    {
        if ( false == kernel::critical_section::init( shared_data.cs_context, 100U))
        {
            kernel::hardware::debug::print( "failed to create critical section\n");
        }
    }

    kernel::task::create( worker_task, kernel::task::Priority::Medium, nullptr, &shared_data);
    kernel::task::create( worker_task, kernel::task::Priority::Medium, nullptr, &shared_data);
    kernel::task::create( worker_task, kernel::task::Priority::Medium, nullptr, &shared_data);

    kernel::start();

    for(;;);
}

void delay( uint32_t a_ticks)
{
    for ( volatile uint32_t i = 0U; i < a_ticks; i++);
}

void worker_task( void * a_parameter)
{
    SharedData & shared_data = *reinterpret_cast< SharedData*>( a_parameter);

    const char * text_id[ 3] = { "task 0: ", "task 1: ", "task 2: "};
    static int task_id = 0;
    const int local_id = task_id;
    ++task_id;

    while ( true)
    {
        if constexpr( use_critical_section)
        {
            kernel::critical_section::enter( shared_data.cs_context);
        }

        changeCaseSize( shared_data);
        kernel::hardware::debug::print( text_id[ local_id]);
        kernel::hardware::debug::print( shared_data.text);

        if constexpr( use_critical_section)
        {
            kernel::critical_section::leave( shared_data.cs_context);
        }

        // Without delay, first task will starve others.
        if ( 0 == local_id)
        {
            delay( 230U);
        }
        else if ( 1 == local_id)
        {
            delay( 130U);
        }
        else
        {
            delay( 900U);
        }
    }
}