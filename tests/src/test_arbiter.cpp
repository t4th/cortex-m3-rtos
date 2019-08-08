#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

#include "arbiter.h"



TEST_CASE("init") {
    arbiter_t arbiter = { 0 };

    Arbiter_Init(&arbiter);

    for (int prio = 0; prio < MAX_PRIORITIES; prio++) {
        for (int task = 0; task < MAX_USER_TASKS; task++) {
            REQUIRE(INVALID_HANDLE == arbiter.task_list[prio].list[task]);
        }
    }
}

TEST_CASE("add items") {
    arbiter_t arbiter = { 0 };

    Arbiter_Init(&arbiter);

    Arbiter_AddTask(&arbiter, T_HIGH, 1);
    Arbiter_AddTask(&arbiter, T_LOW, 2);
    Arbiter_AddTask(&arbiter, T_HIGH, 3);
    Arbiter_AddTask(&arbiter, T_MEDIUM, 4);
    Arbiter_AddTask(&arbiter, T_HIGH, 5);
    Arbiter_AddTask(&arbiter, T_MEDIUM, 6);

    REQUIRE(arbiter.task_list[T_HIGH].count == 3);
        REQUIRE(arbiter.task_list[T_HIGH].list[0] == 1);
        REQUIRE(arbiter.task_list[T_HIGH].list[1] == 3);
        REQUIRE(arbiter.task_list[T_HIGH].list[2] == 5);


    REQUIRE(arbiter.task_list[T_MEDIUM].count == 2);
        REQUIRE(arbiter.task_list[T_MEDIUM].list[0] == 4);
        REQUIRE(arbiter.task_list[T_MEDIUM].list[1] == 6);

    REQUIRE(arbiter.task_list[T_LOW].count == 1);
        REQUIRE(arbiter.task_list[T_LOW].list[0] == 2);
}

TEST_CASE("removing items") {
    arbiter_t arbiter = { 0 };

    Arbiter_Init(&arbiter);

    Arbiter_AddTask(&arbiter, T_HIGH, 1);
    Arbiter_RemoveTask(&arbiter, T_HIGH, 1);

    Arbiter_AddTask(&arbiter, T_LOW, 2);
    Arbiter_RemoveTask(&arbiter, T_LOW, 2);
    // corner-case: removing item that is already removed
    Arbiter_RemoveTask(&arbiter, T_LOW, 2);

    Arbiter_AddTask(&arbiter, T_HIGH, 3);
    Arbiter_RemoveTask(&arbiter, T_HIGH, 3);
    Arbiter_AddTask(&arbiter, T_HIGH, 3);

    Arbiter_AddTask(&arbiter, T_MEDIUM, 4);
    Arbiter_RemoveTask(&arbiter, T_MEDIUM, 4);
    Arbiter_AddTask(&arbiter, T_MEDIUM, 4);

    Arbiter_AddTask(&arbiter, T_HIGH, 5);
    Arbiter_RemoveTask(&arbiter, T_HIGH, 5);
    Arbiter_AddTask(&arbiter, T_HIGH, 5);

    Arbiter_AddTask(&arbiter, T_MEDIUM, 6);
    Arbiter_RemoveTask(&arbiter, T_MEDIUM, 6);
    Arbiter_AddTask(&arbiter, T_MEDIUM, 6);

    REQUIRE(arbiter.task_list[T_HIGH].count == 2);
        REQUIRE(arbiter.task_list[T_HIGH].list[0] == 3);
        REQUIRE(arbiter.task_list[T_HIGH].list[1] == 5);


    REQUIRE(arbiter.task_list[T_MEDIUM].count == 2);
        REQUIRE(arbiter.task_list[T_MEDIUM].list[0] == 4);
        REQUIRE(arbiter.task_list[T_MEDIUM].list[1] == 6);

    REQUIRE(arbiter.task_list[T_LOW].count == 0);
}

TEST_CASE("find next in queue") {
    arbiter_t arbiter = { 0 };
    task_handle_t handle;

    Arbiter_Init(&arbiter);

    Arbiter_AddTask(&arbiter, T_HIGH, 1);
    handle = Arbiter_FindNext(&arbiter, T_HIGH);
    REQUIRE(1 == handle);
    REQUIRE(0 == arbiter.task_list[T_HIGH].current);

    Arbiter_AddTask(&arbiter, T_HIGH, 2);
    handle = Arbiter_FindNext(&arbiter, T_HIGH);
    REQUIRE(2 == handle);
    REQUIRE(1 == arbiter.task_list[T_HIGH].current);

    Arbiter_RemoveTask(&arbiter, T_HIGH, 2);

    handle = Arbiter_FindNext(&arbiter, T_HIGH);
    REQUIRE(1 == handle);
    REQUIRE(0 == arbiter.task_list[T_HIGH].current);


    Arbiter_AddTask(&arbiter, T_HIGH, 3);
    Arbiter_AddTask(&arbiter, T_HIGH, 4);
    Arbiter_AddTask(&arbiter, T_HIGH, 5);


    handle = Arbiter_FindNext(&arbiter, T_HIGH);
    REQUIRE(3 == handle);
    REQUIRE(1 == arbiter.task_list[T_HIGH].current);


    handle = Arbiter_FindNext(&arbiter, T_HIGH);
    REQUIRE(4 == handle);
    REQUIRE(2 == arbiter.task_list[T_HIGH].current);


    handle = Arbiter_FindNext(&arbiter, T_HIGH);
    REQUIRE(5 == handle);
    REQUIRE(3 == arbiter.task_list[T_HIGH].current);

    // back to 1st element
    handle = Arbiter_FindNext(&arbiter, T_HIGH);
    REQUIRE(1 == handle);
    REQUIRE(0 == arbiter.task_list[T_HIGH].current);

    Arbiter_RemoveTask(&arbiter, T_HIGH, 3);

    handle = Arbiter_FindNext(&arbiter, T_HIGH);
    REQUIRE(4 == handle);
    REQUIRE(1 == arbiter.task_list[T_HIGH].current);
}
