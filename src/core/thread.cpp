#include "core/thread.hpp"

namespace rg
{

// Native wrappers for ThreadFunc to conform to API.

void Thread::start(ThreadFunc start_fn, void* arg)
{
#ifdef RG_PLATFORM_WIN32
    this->handle = ::CreateThread( 
        null,
        0,
        start_fn,
        arg,
        0,
        &this->id,
    );
    if (res == HANDLE_INVALID) panic("Failed to start a thread");
#else
    s32 res = ::pthread_create(&this->handle, null, (NativeThreadFunc)start_fn, arg);
    if (res != 0) panic("Failed to start a thread");
    this->id = (u32)res;
#endif
}

void Thread::join()
{
#ifdef RG_PLATFORM_WIN32
    u32 res = ::WaitForSingleObject(this->handle, INFINITE);
    if (res == WAIT_FAILED) panic("Failed to join a thread");
#else
    s32 res = ::pthread_join(this->handle, null);
    if (res != 0) panic("Failed to join a thread");
#endif
}

void Thread::detach()
{
#ifdef RG_PLATFORM_WIN32
    if (!::CloseHandle(this->handle)) panic("Failed to detach a thread");
#else
    s32 res = ::pthread_detach(this->handle);
    if (res != 0) panic("Failed to detach a thread");
#endif
}

void Thread::destroy()
{
#ifdef RG_PLATFORM_WIN32
    if (!::CloseHandle(this->handle)) panic("Failed to destroy a thread");
#endif
}

// Mutex.

void Mutex::init()
{
#ifdef RG_PLATFORM_WIN32
#ifdef RG_MULTI_PROCESS
    this->handle = ::CreateMutex(null, false, null);
    if (this->handle == HANDLE_INVALID) panic("Failed to create mutex");
#else
    ::InitializeCriticalSection(&this->handle);
#endif
#else
    s32 res = ::pthread_mutex_init(&this->handle, null);
    if (res != 0) panic("Failed to create mutex");
#endif
}

void Mutex::lock()
{
#ifdef RG_PLATFORM_WIN32
#ifdef RG_MULTI_PROCESS
    u32 res = ::WaitForSingleObject(this->handle, INFINITE);
    if (res == WAIT_FAILED) panic("Failed to join a thread");
#else
    ::EnterCriticalSection(&this->handle);
#endif
#else
    s32 res = ::pthread_mutex_lock(&this->handle);
    if (res != 0) panic("Failed to lock mutex");
#endif
}

void Mutex::unlock()
{
#ifdef RG_PLATFORM_WIN32
#ifdef RG_MULTI_PROCESS
    if (!::ReleaseMutex(this->handle)) panic("Failed to unlock mutex");
#else
    ::LeaveCriticalSection(&this->handle);
#endif
#else
    s32 res = ::pthread_mutex_unlock(&this->handle);
    if (res != 0) panic("Failed to unlock mutex");
#endif
}

void Mutex::destroy()
{
#ifdef RG_PLATFORM_WIN32
#ifdef RG_MULTI_PROCESS
    if (!::CloseHandle(this->handle)) panic("Failed to destroy a mutex");
#else
    ::DeleteCriticalSection(this->handle);
#endif
#else
    s32 res = ::pthread_mutex_destroy(&this->handle);
    if (res != 0) panic("Failed to destroy mutex");
#endif
}

// Conditions.

void ConditionVariable::init()
{
#ifdef RG_PLATFORM_WIN32
    ::InitializeConditionVariable(&this->handle);
#else
    s32 res = ::pthread_cond_init(&this->handle, null);
    if (res != 0) panic("Failed to init a condition variable");
#endif
}

void ConditionVariable::wait(Mutex* mutex)
{
#ifdef RG_PLATFORM_WIN32
    if (!::SleepConditionVariableCS(&this->handle, mutex, INFINITE))
        panic("Failed to wait on a condition");
#else
    s32 res = ::pthread_cond_wait(&this->handle, &mutex->handle);
    if (res != 0) panic("Failed to wait on a condition");
#endif
}

void ConditionVariable::signal()
{
#ifdef RG_PLATFORM_WIN32
    ::WakeConditionVariable(&this->handle);
#else
    s32 res = ::pthread_cond_signal(&this->handle);
    if (res != 0) panic("Failed to signal on a condition");
#endif
}

void ConditionVariable::broadcast()
{
#ifdef RG_PLATFORM_WIN32
    ::WakeAllConditionVariable(&this->handle);
#else
    s32 res = ::pthread_cond_broadcast(&this->handle);
    if (res != 0) panic("Failed to broadcast on a condition");
#endif
}

void ConditionVariable::destroy()
{
#ifdef RG_PLATFORM_POSIX
    s32 res = ::pthread_cond_destroy(&this->handle);
    if (res != 0) panic("Failed to destroy a condition");
#endif
}

// Semaphore.

void Semaphore::init(sz init_val, sz max_val)
{
#ifdef RG_PLATFORM_WIN32
    ASSERT_MSG(init_val <= max_val, "Must be smaller or equal to max_val");
    this->handle = ::CreateSemaphore(null, init_val, max_val, null);
    if (this->handle == NULL) panic("Failed to initialize semaphore");
#else
    s32 res = ::sem_init(&this->handle, 0, init_val);
    if (res != 0) panic("Failed to initialize semaphore");
#endif
}

void Semaphore::wait()
{
#ifdef RG_PLATFORM_WIN32
    if (::WaitForSingleObject(this->handle, INFINITE) == WAIT_FAILED)
        panic("Failed to wait on semaphore");
#else
    s32 res = ::sem_wait(&this->handle);
    if (res != 0) panic("Failed to wait on semaphore");
#endif
}

void Semaphore::increment(sz count)
{
#ifdef RG_PLATFORM_WIN32
    if (!::ReleaseSemaphore(this->handle, count, null))
        panic("Failed to increment semaphore");
#else
    for (sz i = 0; i < count; ++i)
    {
        s32 res =::sem_post(&this->handle);
        if (res != 0) panic("Failed to increment semaphore");
    }
#endif
}

void Semaphore::destroy()
{
#ifdef RG_PLATFORM_WIN32
    ::CloseHandle(this->handle);
#else
    s32 res = ::sem_destroy(&this->handle);
    if (res != 0) panic("Failed to destroy semaphore");
#endif
}

} // rg
