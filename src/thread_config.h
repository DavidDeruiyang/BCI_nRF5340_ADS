#pragma once

// Cooperative Threads have negative priority
#define HOSTCOMM_THREAD_PRIORITY (-2)
#define HOSTCOMM_THREAD_STACK_SIZE (5096)

#define INTAN_THREAD_PRIORITY (-10)  // -10 priority is higher than -2
#define INTAN_THREAD_STACK_SIZE (2048)

