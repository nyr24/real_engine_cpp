#ifndef _RG_THREAD_HPP_
#define _RG_THREAD_HPP_

#include "core/basic.hpp"

#ifdef RG_PLATFORM_WIN32
alias ThreadHandle = HANDLE;
#else
#include <pthread.h>
#include <semaphore.h>
alias ThreadHandle = u64;
#endif

namespace rg
{

// Thread.

#ifdef RG_PLATFORM_WIN32
alias NativeThreadFunc = u32(*)(void*);
#else
alias NativeThreadFunc = void*(*)(void*);
#endif

alias ThreadFunc = s32(*)(void*);

struct Thread
{
    ThreadHandle handle;
    u32 id;

    Thread() {}
    void start(ThreadFunc start_fn, void* arg);
    void join();
    void detach();
    void destroy();
};

// Mutex.

#ifdef RG_PLATFORM_WIN32
    #ifdef RG_MULTI_PROCESS
    alias NativeMutex = HANDLE;
    #else
    alias NativeMutex = CRITICAL_SECTION;
    #endif
#else
alias NativeMutex = pthread_mutex_t;
#endif

enum struct MutexState
{
    NOT_INITIALIZED,  
    INITIALIZED,
    LOCKED,
};

struct Mutex
{
    NativeMutex handle; 

    void init();
    void lock();
    void unlock();
    void destroy();
};

// Conditions.

#ifdef RG_PLATFORM_WIN32
alias NativeCond = CONDITION_VARIABLE;
#else
alias NativeCond = pthread_cond_t;
#endif

struct ConditionVariable
{
    NativeCond handle;

    void init();
    void wait(Mutex* mutex);
    void signal();
    void broadcast();
    void destroy();
};

// Semaphore

#ifdef RG_PLATFORM_WIN32
alias NativeSemaphore = HANDLE;
#else
alias NativeSemaphore = sem_t;
#endif

struct Semaphore
{
    NativeSemaphore handle;

    void init(sz init_val, sz max_val);
    void wait();
    void increment(sz count = 1);
    void destroy();
};

} // rg

#endif // _RG_THREAD_HPP_
