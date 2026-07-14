#ifndef _RG_CONTEXT_HPP_
#define _RG_CONTEXT_HPP_

#include "core/basic.hpp"
#include "core/thread.hpp"
#include "core/allocators.hpp"

namespace rg
{

struct Context
{
    Allocator* allocator;
    XorshiftRng rng;
    Mutex logger_mutex;

    void init(Allocator* persistent_alloc);
    void destroy();
};

static constexpr sz DEFAULT_TEMP_STORAGE_CAPACITY = 8 * MB;

#define TEMP_ALLOC_SCOPE(arena) \
    sz mark = arena->save_mark(); \
    defer(arena->restore_mark(mark));

void context_init(Allocator* allocator);
void context_destroy();
Context& get_context();
Arena* get_temp_allocator();
void init_temp_allocator(Allocator* backing_alloc, sz capacity = DEFAULT_TEMP_STORAGE_CAPACITY);

} // rg

#endif // _RG_CONTEXT_HPP_


