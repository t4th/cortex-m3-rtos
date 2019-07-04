# rtos
Small cortex-m3 RTOS

This is experimental fun project so no fancy build and tool sets are used. Simple Keil Uvision project.

Goals & assumptions

Scheduler
– Fully pre-emptive
– multitask
– statically defined task before starting OS (no run-time thread creation)
– highest priority task are to be served first
– tasks of the same priority should be running using Round Robin

Synchronization
– events, 2-state SET/RESET
– interrupts can be mapped to events.
– critical section
– WaitForEvent function
– sleep function

Memory
– compile time defined reserved thread and stack space
– no dynamic allocations
