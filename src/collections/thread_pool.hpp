#ifndef _RG_THREAD_POOL_HPP_
#define _RG_THREAD_POOL_HPP_

#include "core/basic.hpp"
#include "core/thread.hpp"
#include "core/bits.hpp"
#include "core/context.hpp"
#include "collections/farray.hpp"
#include "collections/ringbuffer.hpp"

namespace rg
{

struct ThreadTask
{
    ThreadFunc fn;
    void* arg;
};

template<sz THREAD_COUNT, sz MAX_TASKS>
struct ThreadPool;

template<sz THREAD_COUNT, sz MAX_TASKS>
struct WorkerInput
{
    ThreadPool<THREAD_COUNT, MAX_TASKS>* tpool; 
    sz idx;
};

// thread pool version 1: mutex + condvars
#if 1

template<sz THREAD_COUNT, sz MAX_TASKS>
struct ThreadPool
{
    static constexpr u32 INIT_BIT = 1 << 0;
    static constexpr u32 STOP_BIT = 1 << 1;

    Array<Thread, THREAD_COUNT> threads;
    Mutex mutex;
    ConditionVariable work_acquired;
    ConditionVariable work_finished;
    ConditionVariable thread_died;
    RingBuffer<ThreadTask, MAX_TASKS> task_queue;
    sz tasks_in_progress;
    BitInt<u32, BitMask32> active_threads;
    BitInt<u32, BitMask32> init_stop;
    Array<WorkerInput<THREAD_COUNT, MAX_TASKS>> worker_inputs;

    void init();
    void submit_task(const ThreadTask& task);
    void submit_task_many(Slice<ThreadTask> tasks);
    void await();
    void destroy();

    bool is_initialized()
    {
        return this->init_stop.is_set(INIT_BIT);
    }
    void set_is_initialized()
    {
        this->init_stop.set(INIT_BIT);
    }
    bool is_stop_requested()
    {
        return this->init_stop.is_set(STOP_BIT);
    }
    void set_stop()
    {
        this->init_stop.set(STOP_BIT);
    }
};

template<sz THREAD_COUNT, sz MAX_TASKS>
intern s32 worker(void* arg)
{
    auto* worker_input = (WorkerInput<THREAD_COUNT, MAX_TASKS>*)arg;
    ThreadPool<THREAD_COUNT, MAX_TASKS>* tpool = worker_input->tpool;
    sz thread_idx = worker_input->idx;

    tpool->mutex.lock();
    Context* ctx = get_context();
    init_temp_allocator(ctx->allocator);

    for (;;)
    {
        if (tpool->task_queue.is_empty())
        {
            tpool->work_acquired.wait(&tpool->mutex);
        }
        if (tpool->is_stop_requested())
        {
            tpool->active_threads.unset(thread_idx);
            tpool->thread_died.signal();
            tpool->mutex.unlock();
            break;
        }
        // Process the task.
        auto [task, is_ok] = tpool->task_queue.pop_safe();
        if (!is_ok) continue;
        tpool->tasks_in_progress++;
        tpool->mutex.unlock();
        task.fn(task.arg);
        tpool->mutex.lock();
        tpool->tasks_in_progress--;
        tpool->work_finished.signal();
    }

    return 0;
}

template<sz THREAD_COUNT, sz MAX_TASKS>
void ThreadPool<THREAD_COUNT, MAX_TASKS>::init()
{
    this->mutex.init();
    this->work_acquired.init();
    this->work_finished.init();
    this->thread_died.init();
    this->tasks_in_progress = 0;
 
    for (sz i = 0; i < this->threads.len(); ++i)
    {
        this->active_threads.set(i);
        this->worker_inputs[i] = {
            .tpool = this,
            .idx = i,
        };
        Thread& t = this->threads[i];
        t.start(worker<THREAD_COUNT, MAX_TASKS>, &this->worker_inputs[i]);
        t.detach();
    }
}

template<sz THREAD_COUNT, sz MAX_TASKS>
void ThreadPool<THREAD_COUNT, MAX_TASKS>::submit_task(const ThreadTask& task)
{
    this->mutex.lock();
    defer(this->mutex.unlock());
    this->task_queue.push(task);
    this->work_acquired.broadcast();
}

template<sz THREAD_COUNT, sz MAX_TASKS>
void ThreadPool<THREAD_COUNT, MAX_TASKS>::submit_task_many(Slice<ThreadTask> tasks)
{
    this->mutex.lock();
    defer(this->mutex.unlock());
    this->task_queue.push(tasks);
    this->work_acquired.broadcast();
}

template<sz THREAD_COUNT, sz MAX_TASKS>
void ThreadPool<THREAD_COUNT, MAX_TASKS>::await()
{
    this->mutex.lock();
    defer(this->mutex.unlock());
    // TODO: Context->thread_id
    while (this->tasks_in_progress > 0 || !this->task_queue.is_empty())
    {
        this->work_finished.wait(&this->mutex);
    }
}

template<sz THREAD_COUNT, sz MAX_TASKS>
void ThreadPool<THREAD_COUNT, MAX_TASKS>::destroy()
{
    this->mutex.lock();
    defer(this->mutex.unlock());
    // TODO: Context->thread_id
    this->set_stop();

    while (this->active_threads.set_bit_count() != 0)
    {
        this->work_acquired.broadcast();
        this->thread_died.wait(&this->mutex);
    }

    this->active_threads.unset_all();
    this->work_acquired.destroy();
    this->work_finished.destroy();
    this->thread_died.destroy();
    this->tasks_in_progress = 0;
}

// thread pool version 2: atomic + semaphores
#else
#include "core/atomic.hpp"

template<sz THREAD_COUNT, sz MAX_TASKS>
struct ThreadPool
{
    static constexpr u32 INIT_BIT = 1 << 0;
    static constexpr u32 STOP_BIT = 1 << 1;

    Array<Thread, THREAD_COUNT> threads;
    Semaphore work_acquired_sem;
    Semaphore work_finished_sem;
    Semaphore thread_died_sem;
    RingBuffer<ThreadTask, MAX_TASKS> task_queue;
    Atomic<sz> tasks_in_progress;
    AtomicBitInt<u32, BitMask32> active_threads;
    BitInt<u32, BitMask32> init_stop;
    Array<WorkerInput<THREAD_COUNT, MAX_TASKS>, THREAD_COUNT> worker_inputs;

    void init();
    void submit_task(const ThreadTask& task);
    void submit_task_many(Slice<ThreadTask> tasks);
    void await();
    void destroy();

    bool is_initialized()
    {
        return this->init_stop.is_set(INIT_BIT);
    }
    void set_is_initialized()
    {
        this->init_stop.set(INIT_BIT);
    }
    bool is_stop_requested()
    {
        return this->init_stop.is_set(STOP_BIT);
    }
    void set_stop()
    {
        this->init_stop.set(STOP_BIT);
    }
};

template<sz THREAD_COUNT, sz MAX_TASKS>
intern s32 worker(void* arg)
{
    AtomicOrder ord = AtomicOrder::SEQ_CST;
    auto* worker_input = (WorkerInput<THREAD_COUNT, MAX_TASKS>*)arg;
    ThreadPool<THREAD_COUNT, MAX_TASKS>* tpool = worker_input->tpool;
    sz thread_idx = worker_input->idx;

    Context& ctx = get_context();
    init_temp_allocator(ctx.persistent_alloc);

    for (;;)
    {
        if (tpool->task_queue.is_empty())
        {
            tpool->work_acquired_sem.wait();
        }
        if (tpool->is_stop_requested())
        {
            tpool->active_threads.unset(thread_idx);
            tpool->thread_died_sem.increment();
            break;
        }
        // Process the task.
        auto [task, is_ok] = tpool->task_queue.pop_safe();
        if (!is_ok) continue;
        tpool->tasks_in_progress.inc(ord);
        task.fn(task.arg);
        tpool->tasks_in_progress.dec(ord);
        tpool->work_finished_sem.increment();
    }

    return 0;
}

template<sz THREAD_COUNT, sz MAX_TASKS>
void ThreadPool<THREAD_COUNT, MAX_TASKS>::init()
{
    this->work_acquired_sem.init(0, MAX_TASKS);
    this->work_finished_sem.init(0, MAX_TASKS);
    this->thread_died_sem.init(0, THREAD_COUNT);
    this->tasks_in_progress.store(0);

    for (sz i = 0; i < this->threads.len(); ++i)
    {
        this->active_threads.set(i);
        this->worker_inputs[i] = {
            .tpool = this,
            .idx = i,
        };
        Thread& t = this->threads[i];
        t.start(worker<THREAD_COUNT, MAX_TASKS>, &this->worker_inputs[i]);
        t.detach();
    }
}

template<sz THREAD_COUNT, sz MAX_TASKS>
void ThreadPool<THREAD_COUNT, MAX_TASKS>::submit_task(const ThreadTask& task)
{
    this->task_queue.push(task);
    this->work_acquired_sem.increment();
}

template<sz THREAD_COUNT, sz MAX_TASKS>
void ThreadPool<THREAD_COUNT, MAX_TASKS>::submit_task_many(Slice<ThreadTask> tasks)
{
    this->task_queue.push(tasks);
    this->work_acquired_sem.increment(tasks.count);
}

template<sz THREAD_COUNT, sz MAX_TASKS>
void ThreadPool<THREAD_COUNT, MAX_TASKS>::await()
{
    // TODO: Context->thread_id
    while (this->tasks_in_progress.load() > 0 || !this->task_queue.is_empty())
    {
        this->work_finished_sem.wait();
    }
}

template<sz THREAD_COUNT, sz MAX_TASKS>
void ThreadPool<THREAD_COUNT, MAX_TASKS>::destroy()
{
    // TODO: Context->thread_id
    this->set_stop();
    this->work_acquired_sem.increment(THREAD_COUNT);

    while (this->active_threads.set_bit_count() != 0)
    {
        this->thread_died_sem.wait();
    }

    this->active_threads.unset_all();
    this->work_acquired_sem.destroy();
    this->work_finished_sem.destroy();
    this->thread_died_sem.destroy();
    this->tasks_in_progress.store(0);
}

#endif

} // rg

#endif // _RG_THREAD_POOL_HPP_
