# Kernel POC
Branch used to experiment with initial kernel concept, like system objects buffering and system/user APIs. It is test ground to change different apporoaches of API design and solutions to find something fun.

## Note #1 - There is really 'a lot' of decision to be made!
I never wanted create rtos will all possible configurations available - in popular systems it generated this big config-like header with hundreds of options to change via some defines.
I my case, I have this single target and clear vision what i want to make. Yet...
There are so many ways to implement something, that is mind bogling. From data structures to algorithm and APIs decisions!

### Arbiter (or scheduler)
Use case: Cheap add and remove from scheduling queue while keeping the order. Circular linked list with static buffer seems to fit well. Big system (unix) on big hardware (x86/64) use stuff like self sorting BT tree, but in my case number of task running likely wont exceed 20.

### API
Thousands ways to make the same. I decided to use Win32 API handle system to access system objects like events, timers, tasks. It is less error phrone to pass handle that some raw pointers with cost of some little memory.

### System objects buffers
Static. For now. All buffers are defined during compile time since I have cleary defined maximum hardware capability and use cases. Simple API as possible. No advances settings (yet) available, like custom task stack sizes or priviledgs - that would be overcomplication of simple RTOS.
With so little system, with system objects ranging from 5-30, all buffers are simple arrays since searching times are neglicible.

### System memory size
For simplicity reasons, all 'status' bools, are defined as 32 bit words. Further changing to 1 byte (since ARM always fetch 1 word) and even 1 bit state would reduce memory usage. But this is polishing phase, since my target has 512 kb flash memory and no cache memory.
