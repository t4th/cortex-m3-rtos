// Contains example used to verify if tasks are switching correctly.
// Test: Create two tasks trying to access shared data with
// and without critical section.

#include <kernel.hpp>
#include <hardware.hpp>

#include <cctype>
#include <cstring>

// Comment define to not use critical section.
#define USE_CRITICAL_SECTION

void printText(const char * a_text)
{
    kernel::hardware::debug::print(a_text);
}

const size_t MAX = 100;

struct SharedDara
{
#ifdef USE_CRITICAL_SECTION
    kernel::critical_section::Context context;
#endif
    char        text[MAX];
    size_t      size;
};

void changeCaseSize(SharedDara & a_shared)
{
#ifdef USE_CRITICAL_SECTION
    kernel::critical_section::enter(a_shared.context);
#endif

    for (size_t i = 0; i < a_shared.size; ++i)
    {
        if (isupper(a_shared.text[i]))
        {
            a_shared.text[i] = tolower(a_shared.text[i]);
        }
        else
        {
            a_shared.text[i] = toupper(a_shared.text[i]);
        }
    }

#ifdef USE_CRITICAL_SECTION
    kernel::critical_section::leave(a_shared.context);
#endif
}

void worker_task(void * a_parameter);

int main()
{
    SharedDara shared;

    strcpy(shared.text, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n");
    shared.size = strlen(shared.text);

#ifdef USE_CRITICAL_SECTION
    kernel::critical_section::init(shared.context);
#endif

    kernel::init();

    kernel::task::create(worker_task, kernel::task::Priority::Medium, nullptr, &shared);
    kernel::task::create(worker_task, kernel::task::Priority::Medium, nullptr, &shared);
    kernel::task::create(worker_task, kernel::task::Priority::Medium, nullptr, &shared);

    kernel::start();

    for(;;);
}

void worker_task(void * a_parameter)
{
    SharedDara * shared = (SharedDara*) a_parameter;

    while (1)
    {
        changeCaseSize(*shared);
        kernel::hardware::debug::print(shared->text);
    }
}