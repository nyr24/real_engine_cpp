#ifndef _RG_CONTEXT_HPP_
#define _RG_CONTEXT_HPP_

#include "core/basic.hpp"
#include "core/thread.hpp"
#include "core/allocators.hpp"
#include "collections/ringbuffer.hpp"

namespace rg
{

constexpr sz MAX_EXIT_CALLBACKS = 32;

// Global, application-level context.

struct Context
{
    Allocator* allocator;
    XorshiftRng rng;
    Mutex logger_mutex;
    RingBuffer<ExitCallback, MAX_EXIT_CALLBACKS> exit_callbacks;
};

intern constexpr sz DEFAULT_TEMP_STORAGE_CAPACITY = 64 * MB;

#define TEMP_ALLOC_SCOPE(temp_alloc) \
    sz mark = temp_alloc->save_mark(); \
    defer(temp_alloc->restore_mark(mark));

Context* get_context();
Arena* get_temp_allocator();
Allocator* get_persist_allocator();
void init_temp_allocator(Allocator* backing_alloc, sz capacity = DEFAULT_TEMP_STORAGE_CAPACITY);
sz get_thread_id();

} // rg

#endif // _RG_CONTEXT_HPP_
