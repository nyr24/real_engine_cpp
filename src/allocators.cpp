#include "basic.hpp"
#include "allocators.hpp"

namespace rg
{

AllocatorVtable heap_vtable = {
    .allocate = &heap_allocate,  
    .reallocate = &heap_reallocate,
    .free = &heap_free,
    .display_info = &heap_display_info,
};

void* allocator_allocate(Allocator* alloc, sz size, sz alignment, bool zero_mem)
{
    ASSERT_MSG(alloc->vtable != null, "Vtable should exist");
    return alloc->vtable->allocate(alloc, size, alignment, zero_mem);
}

void* allocator_reallocate(Allocator* alloc, void* ptr, sz new_size, sz alignment)
{
    ASSERT_MSG(alloc->vtable != null, "Vtable should exist");
    return alloc->vtable->reallocate(alloc, ptr, new_size, alignment);
}

void allocator_free(Allocator* alloc, void* ptr)
{
    ASSERT_MSG(alloc->vtable != null, "Vtable should exist");
    return alloc->vtable->free(alloc, ptr);
}

void allocator_display_info(Allocator* alloc)
{
    ASSERT_MSG(alloc->vtable != null, "Vtable should exist");
    return alloc->vtable->display_info(alloc);
}

void heap_init(HeapAlloc* self)
{
    self->vtable = &heap_vtable;
}

void* heap_allocate(Allocator* self, sz size, sz alignment, bool zero_mem)
{
    ASSERT_MSG(size >= 0, "Allocation size should be greater than zero");
    
    if (alignment > 0)
    {
        alignment = alignment_for_allocation(alignment);
        void* unaligned_ptr = malloc(size + alignment);  
        void* aligned_ptr = align_ptr(unaligned_ptr, alignment);
        if (zero_mem) mem_zero(aligned_ptr, size);
        return aligned_ptr;
    }
    else
    {
        void* ret = malloc(size);
        if (zero_mem) mem_zero(ret, size);
        return ret;
    }
}

void heap_free(Allocator* self, void* ptr)
{
    ASSERT_MSG(ptr != null, "Pointer to free mustn't be null");
    free(ptr);
    ptr = null;
}

void* heap_reallocate(Allocator* self, void* ptr, sz new_size, sz alignment)
{
    ASSERT_MSG(ptr != null, "Pointer must be non-null");
    ASSERT_MSG(new_size > 0, "New size must be greater than zero");

    if (alignment)
    {
        alignment = alignment_for_allocation(alignment);
        ptr = realloc(ptr, new_size + alignment);
        align_ptr(ptr, alignment);
        return ptr;
    }
    else
    {
        return realloc(ptr, new_size);
    }
}

void heap_display_info(Allocator* self) {}

// Arena allocator.

AllocatorVtable arena_vtable = {
    .allocate = &arena_allocate,
    .reallocate = &arena_reallocate,
    .free = &arena_free,
    .display_info = &arena_display_info,
};

Arena* arena_create(Allocator* backing_alloc, sz init_capacity)
{
    ASSERT_MSG(init_capacity > 0, "Must be greater than 0");

    Arena* arena = (Arena*)backing_alloc->vtable->allocate(backing_alloc, sizeof(Arena) + init_capacity, 0, false);
    arena->vtable = &arena_vtable;
    arena->cursor = 0;
    arena->capacity = init_capacity;
    arena->backing_alloc = backing_alloc;
    arena->mark_count = 0;
    arena->fallback_root = null;
    return arena;
}

void* arena_allocate(Allocator* self, sz size, sz alignment, bool zero_mem)
{
    ASSERT_MSG(size >= 0, "Allocation size should be greater than zero");
    
    Arena* arena = (Arena*)self;
    alignment = alignment_for_allocation(alignment);
    size = align(size, DEFAULT_MEM_ALIGNMENT);

    if (size + alignment + ARENA_HEADER_SIZE > arena->remain_mem())
    {
        return arena->fallback_allocate(size, alignment, zero_mem);
    }

    u8* curr = arena->cursor_ptr();
    u8* aligned = align_ptr(curr + ARENA_HEADER_SIZE, alignment);
    sz* header = (sz*)(aligned - ARENA_HEADER_SIZE);
    *header = size;

    if (zero_mem) mem_zero(aligned, size);
    arena->cursor = (uptr(aligned) - uptr(arena)) + size;
    return aligned;
}

// Use of this is discouraged, prefer to use 'marks' to free arena memory all at once.
void arena_free(Allocator* self, void* ptr)
{
    ASSERT_MSG(ptr != null, "Pointer to free mustn't be null");

    Arena* arena = (Arena*)self;
    
    if (!arena->owns_ptr(ptr)) return;

    sz* old_size = (sz*)((u8*)ptr - ARENA_HEADER_SIZE);
    if (!arena->is_last_alloc(arena->cursor, ptr, *old_size)) return;

    arena->cursor -= *old_size + ARENA_HEADER_SIZE;
}

void* arena_reallocate(Allocator* self, void* ptr, sz new_size, sz alignment)
{
    Arena* arena = (Arena*)self;

    if (!ptr) return arena_allocate(self, new_size, alignment, false);
    if (new_size == 0) arena_free(self, ptr);
    
	sz aligned_new_size = align(new_size, DEFAULT_MEM_ALIGNMENT);
	sz* old_size        = (sz*)((u8*)ptr - ARENA_HEADER_SIZE);
	sz alloc_size_diff  = aligned_new_size - *old_size;
    
    bool last_alloc = arena->is_last_alloc(arena->cursor, ptr, *old_size);
    if (last_alloc && alloc_size_diff <= arena->remain_mem())
    {
    	// Extend allocation.
    	arena->cursor += alloc_size_diff;
    	return ptr;
    }
    // Do fresh allocation.
    void* ret = arena_allocate(self, new_size, alignment, false);
    mem_copy((u8*)ret, (u8*)ptr, min(*old_size, new_size));
    return ret;
}

void arena_display_info(Allocator* self)
{
    TODO("display arena allocator info!");
}

void* Arena::fallback_allocate(sz size, sz alignment, bool zero_mem)
{
    FallbackAllocation* curr = this->fallback_root;
    while (curr)
    {
        curr = curr->next;
    }

    curr = (FallbackAllocation*)this->backing_alloc->vtable->allocate(this->backing_alloc, size + sizeof(FallbackAllocation), alignment, zero_mem);
    curr->size = size;
    curr->next = null;

    if (this->fallback_root == null) this->fallback_root = curr;

    return curr->memory();
}

void Arena::fallback_free_all()
{
    ASSERT_MSG(this->backing_alloc, "Backing allocator must be available");

    FallbackAllocation* curr = this->fallback_root;
    FallbackAllocation* next;

    while (curr)
    {
        next = curr->next;
        allocator_free(this->backing_alloc, curr);
        curr = next;
    }

    this->fallback_root = null;
}

sz Arena::save_mark()
{
    this->mark_count++;
    return this->cursor;
}

void Arena::restore_mark(sz mark)
{
    this->cursor = mark;
    this->mark_count--;
}

bool Arena::owns_ptr(void* ptr)
{
    ASSERT(ptr != null);
    u8* start = this->memory();
    return ptr >= start && ptr <= (start + this->cursor);
}

bool Arena::is_last_alloc(sz cursor, void* ptr, sz alloc_size)
{
	sz alloc_cursor = ((u8*)ptr + alloc_size) - (u8*)this->memory();
	return alloc_cursor == cursor;
}

void Arena::destroy()
{
    ASSERT_MSG(this->backing_alloc, "Backing allocator must be available");
    this->fallback_free_all();
    allocator_free(this->backing_alloc, this);
}

} // rg
