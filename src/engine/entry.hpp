#ifndef _RG_CONTEXT_HPP_
#define _RG_CONTEXT_HPP_

#include "core/basic.hpp"
#include "core/thread.hpp"
#include "core/allocators.hpp"
#include "engine/event.hpp"
#include "engine/input.hpp"

namespace rg
{

void application_init();
void application_run();
void application_destroy();

// Global, application-level context.

struct Context
{
    Allocator* allocator;
    XorshiftRng rng;
    Mutex logger_mutex;

    void init(Allocator* persistent_alloc);
    void destroy();
};

intern constexpr sz DEFAULT_TEMP_STORAGE_CAPACITY = 8 * MB;

#define TEMP_ALLOC_SCOPE(arena) \
    sz mark = arena->save_mark(); \
    defer(arena->restore_mark(mark));

Context* get_context();
Arena* get_temp_allocator();
void init_temp_allocator(Allocator* backing_alloc, sz capacity = DEFAULT_TEMP_STORAGE_CAPACITY);

// Engine context.

struct EngineContext
{
    EventSystem event_sys;
    InputSystem input_sys;
    
    void init();
    void destroy();
};

EngineContext* get_engine_context();

} // rg

#endif // _RG_CONTEXT_HPP_


