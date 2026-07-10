#include "core/basic.hpp"
#include "core/allocators.hpp"

namespace rg
{
    
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

// Vmem allocator.

intern void* virtual_alloc(sz size)
{
#ifdef RG_PLATFORM_WIN32
    u8* res = (u8*)::VirtualAlloc(null, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!res) panic("Failed to allocate memory with VirtualAlloc");
    // Cause page faults for all memory pages, to get them in RAM.
    for (sz i = 0; i < size; i += RG_PAGE_SIZE)
    {
        res[i] = 0;
    }
    return (void*)res;
#else
    void* res = ::mmap(null, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
    if (res == MAP_FAILED) panic("Failed to mmap memory");
    return res;
#endif
}

intern void virtual_free(void* ptr, sz size)
{
#ifdef RG_PLATFORM_WIN32
    if (!::VirtualFree(ptr, size, MEM_RELEASE))
    {
        LOG_WARN("::VirtualFree failed, reason: %u", ::GetLastError());
    }
#else
    if (::munmap(ptr, size) < 0)
    {
        LOG_WARN("::munmap failed");
    }
#endif
}

const AllocatorVtable VMEM_VTABLE = {
    .allocate = &vmem_allocate,
    .reallocate = &vmem_reallocate,
    .free = &vmem_free,
    .display_info = &vmem_display_info,
};

VmemAllocator* VmemAllocator::create(sz init_capacity)
{
    init_capacity = rg::max(init_capacity, MIN_CAPACITY);
    auto* vmem = (VmemAllocator*)virtual_alloc(init_capacity + sizeof(VmemAllocator));
    vmem->vtable = &VMEM_VTABLE;
    vmem->capacity = init_capacity;
    vmem->fallback_root = null;
    vmem->reset();
    return vmem;
}

void VmemAllocator::reset()
{
    auto* root = (VmemFreeNode*)this->memory();
    root->set_size_and_is_free(this->capacity, true);
    root->prev_phys = null;
    root->prev_free = null;
    root->next_free = null;
    this->free_root = root;
    this->used = 0;
}

void VmemAllocator::destroy()
{
    this->fallback_free_all();
    virtual_free(this, this->capacity + sizeof(VmemAllocator));
}

void* vmem_allocate(Allocator* self, sz size_alloc, sz alignment, bool zero_mem)
{
    ASSERT_GREATER_ZERO(size_alloc);
    alignment = alignment_for_allocation(alignment);
    ASSERT_POW_OF_TWO(alignment);

    auto* vmem = (VmemAllocator*)self;
    if (!vmem->free_root)
    {
        LOG_WARN("Virtual allocator is out of main memory");
        return vmem->fallback_allocate(size_alloc, alignment);
    }

    // We need to align it for the next node possible placement.
    size_alloc = align(size_alloc, alignof(VmemFreeNode));
    u64 max_needed_size = u64(alignment) + u64(size_alloc);

    VmemFreeNode* enough_node = vmem->find_best_node(max_needed_size);
    if (!enough_node) return vmem->fallback_allocate(size_alloc, alignment);

    u8* ptr = enough_node->mem_begin();
    u8* aligned_ptr = align_ptr(ptr); 
    auto* aligned_node = (VmemFreeNode*)(aligned_ptr - sizeof(VmemAllocHeader));
    vmem->remove_from_free_list(aligned_node);
    auto* alloc_header = (VmemAllocHeader*)aligned_node;

    // Divide node if possible.
    u8* alloc_end = aligned_ptr + size_alloc;
    u8* enough_node_mem_end = enough_node->mem_end();
    sz remain_space = enough_node_mem_end - alloc_end;
    bool can_add_new_node = remain_space >= VmemAllocator::MIN_ALLOC_SIZE;

    // Insert new free node if have enough space.
    if (can_add_new_node)
    {
        alloc_header->set_size(size_alloc);
        auto* new_free_node = (VmemFreeNode*)alloc_header->next_phys();
        new_free_node->init(aligned_node, remain_space);
        // Need to check if it can be merged with the next block only.
        vmem->insert_after_divide(new_free_node);
    }
    else
    {
        // Reset the size of allocation to whole node size.
        // Anyway we're not inserting a new node.
        alloc_header->set_size(enough_node_mem_end - aligned_ptr);
    }

    // Aligning will possibly move Freenode / AllocHeader pointer
    // So we need to update 'prev_phys' on the next node if it exist,
    // And update the size on the previous node if it exist.
    if (aligned_ptr != ptr)
    {
        auto* prev_node = (VmemFreeNode*)enough_node->prev_phys;
        if (prev_node)
        {
            // Increase prev node size.
            prev_node->set_size(prev_node->size() + sz(aligned_ptr - ptr));
        }
        auto* next_node = (VmemFreeNode*)enough_node_mem_end;
        if (next_node < vmem->tail())
        {
            next_node->prev_phys = alloc_header; 
        }
    }

    return aligned_ptr;
}

void vmem_free(Allocator* self, void* ptr)
{
    ASSERT_NON_NULL(ptr);
    auto* vmem = (VmemAllocator*)self; 
    auto* free_node = (VmemFreeNode*)((u8*)ptr - sizeof(VmemAllocHeader));
    vmem->insert_after_free(free_node);
}

void* vmem_reallocate(Allocator* self, void* ptr, sz new_size, sz alignment)
{
    ASSERT_NON_NULL(ptr);
    ASSERT_GREATER_ZERO(new_size);

    auto* vmem = (VmemAllocator*)self;
    new_size = align(new_size, alignof(VmemFreeNode));
    auto* alloc_header = (VmemAllocHeader*)((u8*)ptr - sizeof(VmemAllocHeader));
    sz old_size = (sz)alloc_header->size();
    sz size_diff = new_size - old_size;

    // Shrink.
    if (size_diff < 0)
    {
        alloc_header->set_size(old_size + size_diff);
        return ptr;
    }

    // Grow or move allocation.
    // We can grow current block only if next block is free, so we can chop memory from it.
    auto* next_node_before_move = (VmemFreeNode*)alloc_header->next_phys();
    bool can_grow_existing = next_node_before_move->is_free();

    if (can_grow_existing)
    {
        vmem->remove_from_free_list(next_node_before_move);
        alloc_header->set_size(old_size + size_diff);
        auto* next_node_after_move = (VmemFreeNode*)alloc_header->next_phys();
        next_node_after_move->init((VmemFreeNode*)alloc_header, next_node_before_move->size() - size_diff);
        // Because we're moving the next node, we need to move prev_phys pointer of its own next node.
        next_node_before_move->next_phys()->prev_phys = next_node_after_move;
        vmem->insert_to_free_list(next_node_after_move);
        return ptr;
    }

    // Can't grow existing.
    // Always align to atleast 2 * sizeof(ptr), so its easier for compiler to perform auto-vectorization.
    alignment = alignment_for_allocation(alignment);
    void* new_allocation = vmem_allocate(self, new_size, alignment, false);
    mem_copy(new_allocation, alloc_header, rg::min(old_size, new_size));
    vmem_free(self, ptr);
    return new_allocation;
}

void vmem_display_info(Allocator *self)
{
    auto* vmem = (VmemAllocator*)self;
    sz fallback_allocated = vmem->calc_fallback_allocated();
    LOG_INFO("Vmem allocator info:");
    LOG_INFO("capacity: %ll, used: %ll, fallback allocated: %ll",
        vmem->capacity, vmem->used, fallback_allocated);
}

/*
 Checks both (next and prev) nodes for merge-ability.
 should be used after 'free' call.
*/
void VmemAllocator::insert_after_free(VmemFreeNode* node)
{
    VmemFreeNode* prev_node = (VmemFreeNode*)node->prev_phys;
    VmemFreeNode* next_node = (VmemFreeNode*)node->next_phys();
    bool should_merge_prev = prev_node != null && prev_node->is_free();
    bool should_merge_next = next_node < this->tail() && next_node->is_free();

    if (!should_merge_prev && !should_merge_next)
    {
        this->insert_to_free_list(node);
        return;
    }

    // If should merge both.
    // Input node doesn't need to be inserted, it gets consumed by prev.
    if (should_merge_next && should_merge_prev)
    {
        this->remove_from_free_list(next_node);
        prev_node->set_size(prev_node->size() + node->space() + next_node->space());
        VmemFreeNode* new_next_phys = prev_node->next_phys();
        if (new_next_phys < this->tail())
        {
            new_next_phys->prev_phys = prev_node;
        }
    }
    // Merge prev only.
    // Input node doesn't need to be inserted, it gets consumed by prev.
    else if (should_merge_prev)
    {
        prev_node->set_size(prev_node->size() + node->space());
        if (next_node < this->tail())
        {
            next_node->prev_phys = prev_node;
        }
    }
    else
    {
        this->remove_from_free_list(next_node);
        node->set_size(node->size() + next_node->space());
        VmemFreeNode* new_next_phys = node->next_phys();
        if (new_next_phys < this->tail())
        {
            new_next_phys->prev_phys = node;
        }
        this->insert_to_free_list(node);
    }
}

/*
 Checks only next node for merge-ability.
 because previous one is being allocated.
*/
void VmemAllocator::insert_after_divide(VmemFreeNode* node)
{
    VmemFreeNode* next_node = (VmemFreeNode*)node->next_phys();
    bool should_merge_next = next_node < this->tail() && next_node->is_free();

    if (!should_merge_next)
    {
        this->insert_to_free_list(node);
        return;
    }

    this->remove_from_free_list(next_node);
    node->set_size(node->size() + next_node->space());
    VmemFreeNode* new_next_phys = node->next_phys();
    if (new_next_phys < this->tail())
    {
        new_next_phys->prev_phys = node;
    }
    this->insert_to_free_list(node);
}

void VmemAllocator::insert_to_free_list(VmemFreeNode* node)
{
    node->set_is_free(true);
    node->next_free = this->free_root;
    if (this->free_root != null)
    {
        this->free_root->prev_free = node;
    }
    this->free_root = node;
}

void VmemAllocator::remove_from_free_list(VmemFreeNode* node)
{
    node->set_is_free(false);
    VmemFreeNode* prev = node->prev_free; 
    VmemFreeNode* next = node->next_free; 

    if (prev)
    {
        prev->next_free = next;
        if (next) next->prev_free = prev;
    }
    // It was a root.
    else
    {
        if (next)
        {
            this->free_root = next;
            if (this->free_root) this->free_root->prev_free = null;
        }
        else this->free_root = null;
    }
}

VmemFreeNode* VmemAllocator::find_best_node(sz need_size)
{
    if (!this->free_root) return null;
    
    VmemFreeNode* best = this->free_root;
    VmemFreeNode* curr = best->next_free; 

    while (curr)
    {
        sz curr_size = curr->size();
        if (curr_size >= need_size && curr_size < best->size())
        {
            best = curr;
        }
        curr = curr->next_free;
    }

    return best;
}


VmemFallbackRegion* VmemFallbackRegion::create(sz capacity)
{
    auto* res = (VmemFallbackRegion*)virtual_alloc(capacity);
    res->capacity = capacity;
    res->cursor = 0;
    res->next = null;
    return res;
}

VmemFallbackRegion* VmemAllocator::find_best_fallback_region(sz need_size)
{
    VmemFallbackRegion* curr = this->fallback_root; 
    VmemFallbackRegion* prev = null; 

    while (curr)
    {
        if (curr->remain_mem() >= need_size) return curr;
        prev = curr;
        curr = curr->next;
    }

    // We stopped at the root.
    if (!prev)
    {
        this->fallback_root->next = VmemFallbackRegion::create(rg::max(need_size, VmemFallbackRegion::DEFAULT_CAPACITY));
        return this->fallback_root->next;
    }

    prev->next = VmemFallbackRegion::create(rg::max(need_size, VmemFallbackRegion::DEFAULT_CAPACITY));
    return prev->next;
}

void* VmemAllocator::fallback_allocate(sz size, sz alignment, bool zero_mem)
{
    size = align(size, DEFAULT_MEM_ALIGNMENT);
    alignment = alignment_for_allocation(alignment);
    sz max_need_size = size + alignment;
    VmemFallbackRegion* enough_reg = this->find_best_fallback_region(max_need_size);
    u8* ptr = enough_reg->mem_begin();
    u8* aligned_ptr = align_ptr(ptr, alignment);
    enough_reg->cursor += (aligned_ptr - ptr) + size;
    return aligned_ptr;
}

void VmemAllocator::fallback_free_all()
{
    VmemFallbackRegion* curr = this->fallback_root;
    VmemFallbackRegion* next;

    while (curr)
    {
        next = curr->next;
        virtual_free(curr, curr->capacity); 
        curr = next;
    }

    this->fallback_root = null;
}

sz VmemAllocator::calc_fallback_allocated()
{
    sz res = 0;
    VmemFallbackRegion* curr = this->fallback_root;
    while (curr)
    {
       res += curr->cursor; 
    }
    return res;
}

// Heap allocator.

const AllocatorVtable HEAP_VTABLE = {
    .allocate = &heap_allocate,  
    .reallocate = &heap_reallocate,
    .free = &heap_free,
    .display_info = &heap_display_info,
};

void HeapAlloc::init()
{
    this->vtable = &HEAP_VTABLE;
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

const AllocatorVtable ARENA_VTABLE = {
    .allocate = &arena_allocate,
    .reallocate = &arena_reallocate,
    .free = &arena_free,
    .display_info = &arena_display_info,
};

Arena* Arena::create(Allocator* backing_alloc, sz init_capacity)
{
    ASSERT_MSG(init_capacity > 0, "Must be greater than 0");

    Arena* arena = (Arena*)allocator_allocate(backing_alloc, sizeof(Arena) + init_capacity);
    arena->vtable = &ARENA_VTABLE;
    arena->cursor = 0;
    arena->capacity = init_capacity;
    arena->backing_alloc = backing_alloc;
    arena->mark_count = 0;
    arena->fallback_root = null;
    return arena;
}

void* arena_allocate(Allocator* self, sz size, sz alignment, bool zero_mem)
{
    ASSERT_GREATER_ZERO(size);
    alignment = alignment_for_allocation(alignment);
    ASSERT_POW_OF_TWO(alignment);
 
    Arena* arena = (Arena*)self;
    size = align(size, DEFAULT_MEM_ALIGNMENT);

    if (size + alignment + sizeof(ArenaAllocHeader) > arena->remain_mem())
    {
        return arena->fallback_allocate(size, alignment, zero_mem);
    }

    u8* curr = arena->cursor_ptr();
    u8* aligned = align_ptr(curr + sizeof(ArenaAllocHeader), alignment);
    ASSERT_ALIGNED(aligned, alignment);
    ArenaAllocHeader* header = (ArenaAllocHeader*)(aligned - sizeof(ArenaAllocHeader));
    u8 padding = u8(uptr(header) - uptr(curr));
    header->set_size(u64(size));
    header->set_padding(padding);

    if (zero_mem) mem_zero(aligned, size);
    arena->cursor += padding + sizeof(ArenaAllocHeader) + u64(size);
    return aligned;
}

// Use of this is discouraged, prefer to use 'marks' to free arena memory all at once.
void arena_free(Allocator* self, void* ptr)
{
    ASSERT_MSG(ptr != null, "Pointer to free mustn't be null");

    Arena* arena = (Arena*)self;
    if (!arena->owns_ptr(ptr)) return;
    ArenaAllocHeader* header = (ArenaAllocHeader*)((u8*)ptr - sizeof(ArenaAllocHeader));
    auto [size, padding] = header->size_and_padding();
    if (!arena->is_last_alloc(arena->cursor, ptr, size)) return;
    arena->cursor -= (padding + sizeof(ArenaAllocHeader) + size);
}

void* arena_reallocate(Allocator* self, void* ptr, sz new_size, sz alignment)
{
    ASSERT_GREATER_ZERO(new_size);
    ASSERT_POW_OF_TWO(alignment);

    Arena* arena = (Arena*)self;

    if (!ptr) return arena_allocate(self, new_size, alignment, false);
    if (new_size == 0) arena_free(self, ptr);

	sz aligned_new_size = align(new_size, DEFAULT_MEM_ALIGNMENT);
	ArenaAllocHeader* header = (ArenaAllocHeader*)((u8*)ptr - sizeof(ArenaAllocHeader));
	u64 old_size = header->size();
	sz alloc_size_diff = aligned_new_size - old_size;
    
    bool last_alloc = arena->is_last_alloc(arena->cursor, ptr, old_size);
    if (last_alloc && alloc_size_diff <= arena->remain_mem())
    {
    	// Extend allocation.
    	arena->cursor += alloc_size_diff;
    	header->set_size(old_size + u64(alloc_size_diff));
    	return ptr;
    }

    // Do fresh allocation.
    void* ret = arena_allocate(self, new_size, alignment, false);
    mem_copy((u8*)ret, (u8*)ptr, rg::min(old_size, u64(new_size)));
    return ret;
}

void arena_display_info(Allocator* self)
{
    Arena* arena = (Arena*)self;
    LOG_DEBUG("Arena allocator info:");
    LOG_DEBUG("capacity: %ll, cursor: %ll, mark count: %ll",
        arena->capacity, arena->cursor, arena->mark_count);
}

void* Arena::fallback_allocate(sz size, sz alignment, bool zero_mem)
{
    FallbackAllocation* curr = this->fallback_root;
    while (curr)
    {
        curr = curr->next;
    }

    curr = (FallbackAllocation*)allocator_allocate(this->backing_alloc, size + sizeof(FallbackAllocation), alignment);
    curr->size = u64(size);
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
