#ifndef _RG_ALLOCATORS_HPP_
#define _RG_ALLOCATORS_HPP_

#include "basic.hpp"

namespace rg
{

struct FallbackAllocation
{
    sz size;
    FallbackAllocation* next;

    inline u8* memory() { return (u8*)this + sizeof(FallbackAllocation); };
};

// Heap allocator.

struct HeapAlloc final : Allocator
{
};

void heap_init(HeapAlloc* self);
void* heap_allocate(Allocator* self, sz size, sz alignment = 0, bool zero_mem = false);
void* heap_reallocate(Allocator* self, void* ptr, sz new_size, sz alignment = 0);
void heap_free(Allocator* self, void* ptr);
void heap_display_info(Allocator* self);

// Arena allocator.

// This should be updated, if new data members added to Arena structure.
const sz ARENA_HEADER_SIZE = sizeof(sz);
internal const sz ARENA_DEFAULT_CAPACITY = 4096;

struct Arena final : Allocator
{
    sz capacity;
    sz cursor;
    sz mark_count;
    Allocator* backing_alloc;
    FallbackAllocation* fallback_root;

    void destroy();
    sz save_mark();
    void restore_mark(sz mark);

    inline u8* memory() { return (u8*)this + sizeof(Arena); }
    inline sz remain_mem() { return this->capacity - this->cursor; }
    inline u8* cursor_ptr() { return (u8*)this + this->cursor; }
    bool owns_ptr(void* ptr);
    bool is_last_alloc(sz cursor, void* ptr, sz alloc_size);
    void* fallback_allocate(sz size, sz alignment = 0, bool zero_mem = 0);
    void fallback_free_all();
};

Arena* arena_create(Allocator* backing_alloc, sz init_capacity = ARENA_DEFAULT_CAPACITY);
void* arena_allocate(Allocator* self, sz size, sz alignment = 0, bool zero_mem = false);
void* arena_reallocate(Allocator* self, void* ptr, sz new_size, sz alignment = 0);
void arena_free(Allocator* self, void* ptr);
void arena_display_info(Allocator* self);

} // rg

#endif // _RG_ALLOCATORS_HPP_
