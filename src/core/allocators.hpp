#ifndef _RG_ALLOCATORS_HPP_
#define _RG_ALLOCATORS_HPP_

#include "core/basic.hpp"
#include "core/thread.hpp"
#include "collections/bits.hpp"

namespace rg
{

struct FallbackAllocation
{
    u64 size;
    FallbackAllocation* next;

    u8* mem_begin() { return (u8*)this + sizeof(*this); };
};

// Vmem Allocator.

// Inserted on allocation for Vmem allocator.
struct VmemAllocHeader
{
    // msb[56=size..8=is_free]lsb
    BitInt<u64> bits;
    VmemAllocHeader* prev_phys;

    sz size() { return (sz)this->bits.get_mask(BIT_MASK_64_HIGH_56); }
    bool is_free() { return (bool)this->bits.get_mask(BIT_MASK_64_LOW_8); }
    Pair<sz, bool> size_and_is_free()
    {
        return { this->size(), this->is_free() };
    }
    void set_size(sz new_size) { this->bits.set_mask(u64(new_size), BIT_MASK_64_HIGH_56); }
    void set_is_free(bool new_free) { this->bits.set_mask(new_free, BIT_MASK_64_LOW_8); } 
    void set_size_and_is_free(sz new_size, bool new_free)
    {
        this->set_size(new_size);
        this->set_is_free(new_free);
    }

    u8* mem_begin() { return (u8*)this + sizeof(*this); };
    u8* mem_end() { return this->mem_begin() + this->size(); };
    // All space taken by this node up until next node.
    sz space() { return this->size() + sizeof(VmemAllocHeader); }
    VmemAllocHeader* next_phys() { return (VmemAllocHeader*)this->mem_end(); }
};

struct VmemFreeNode : VmemAllocHeader
{
    // This data removed on allocation.
    // So, we're not wasting memory here.
    VmemFreeNode* prev_free;
    VmemFreeNode* next_free;

    void init(VmemFreeNode* prev_phys, sz size)
    {
        this->prev_phys = prev_phys;
        this->set_size_and_is_free(size, true); 
    }
    VmemFreeNode* next_phys() { return (VmemFreeNode*)VmemAllocHeader::next_phys(); }
};

/*
 Just an arena, that uses virtual_alloc to get memory.
 Individual free's are not supported on this.
 All fallback memory gets freed at allocator destruction.
 They're just exist because we don't want to crush when we've used all of the main memory.
*/
struct VmemFallbackRegion
{
    static constexpr sz DEFAULT_CAPACITY = RG_PAGE_SIZE;

    sz capacity;
    sz cursor;
    VmemFallbackRegion* next;

    static VmemFallbackRegion* create(sz capacity);
    sz remain_mem() { return this->capacity - this->cursor; }
    u8* mem_begin() { return (u8*)this + sizeof(*this); };
    u8* mem_used_end() { return this->mem_begin() + this->cursor; };
    u8* mem_end() { return this->mem_begin() + this->capacity; };
};

/*
 Thread-safe Virtual memory allocator.
 Allocated contiguously on the heap (metadata + storage)
 Implemented as a Free-list allocator:
 alloc = O(n)
 any allocation size is allowed
 freeing from arbitrary place is allowed
*/
struct VmemAllocator final : Allocator
{
    static constexpr sz DEFAULT_CAPACITY = 2 * GB;
    static constexpr sz MIN_CAPACITY = RG_PAGE_SIZE;
    static constexpr sz MIN_ALLOC_SIZE = sizeof(u64) + 16;
    static constexpr sz MIN_FALLBACK_ALLOC_SIZE = RG_PAGE_SIZE;

    sz capacity;
    VmemFreeNode* free_root;
    VmemFallbackRegion* fallback_root;
    Mutex mutex;
 
    static VmemAllocator* create(sz init_cap = DEFAULT_CAPACITY);
    void reset();
    void destroy();
    // This version checks both, prev and next node for merge possibility. 
    void insert_after_free(VmemFreeNode* node);
    // This version doesn't need to merge previous node, because it is being allocated.
    void insert_after_divide(VmemFreeNode* node);
    void insert_to_free_list(VmemFreeNode* node);
    void remove_from_free_list(VmemFreeNode* node);
    VmemFreeNode* find_best_node(sz need_size);
    VmemFallbackRegion* find_best_fallback_region(sz need_size);
    void* fallback_allocate(sz size, sz alignment = 0, bool zero_mem = false);
    void fallback_free_all();
    sz calc_fallback_allocated();
    
    u8* mem_begin() { return (u8*)this + sizeof(*this); };
    VmemFreeNode* mem_end() { return (VmemFreeNode*)(this->mem_begin() + this->capacity); }
};

void* vmem_allocate(Allocator* self, sz size, sz alignment = 0, bool zero_mem = false);
void* vmem_reallocate(Allocator* self, void* ptr, sz new_size, sz alignment = 0);
bool vmem_resize(Allocator* self, void* ptr, sz new_size, sz alignment = 0);
void vmem_free(Allocator* self, void* ptr);
void vmem_display_info(Allocator* self);

// Heap allocator - wrapper over c-stdlib allocator
// Only used for testing, so ASan and other tools can catch bugs better.

struct HeapNode
{
    HeapNode* next;
private:
    BitInt<u64> bits;
public:
    u64 size() { return this->bits.get_mask(BIT_MASK_64_HIGH_56); }
    u8 padding() { return (u8)this->bits.get_mask(BIT_MASK_64_LOW_8); };
    Pair<u64, u8> size_and_padding() { return { this->size(), this->padding() }; }

    void set_size(u64 new_size) { this->bits.set_mask(new_size, BIT_MASK_64_HIGH_56); }
    void set_padding(u8 new_padding) { this->bits.set_mask(u64(new_padding), BIT_MASK_64_LOW_8); };
    void set_size_and_padding(u64 size, u8 padding)
    {
        this->set_size(size);
        this->set_padding(padding);
    }
    u8* mem_begin() { return (u8*)(this + sizeof(*this) + this->padding()); };
    u8* mem_end() { return (u8*)this->mem_begin() + this->size(); };
};

struct HeapAlloc final : Allocator
{
    static constexpr sz DEFAULT_CAPACITY = 4096;
    
    HeapNode* root;
    Mutex mutex;

    void init();
    void destroy();
};

void* heap_allocate(Allocator* self, sz size, sz alignment = 0, bool zero_mem = false);
void* heap_reallocate(Allocator* self, void* ptr, sz new_size, sz alignment = 0);
bool heap_resize(Allocator* self, void* ptr, sz new_size, sz alignment = 0);
void heap_free(Allocator* self, void* ptr);
void heap_display_info(Allocator* self);

// Arena allocator.

// Padding for simd types can be up to 64 bytes, so we must account for that.
// Layout: [7bytes=size..1byte=padding].
struct ArenaAllocHeader
{
private:
    BitInt<u64> bits;
public:
    u64 size() { return this->bits.get_mask(BIT_MASK_64_HIGH_56); }
    u8 padding() { return (u8)this->bits.get_mask(BIT_MASK_64_LOW_8); };
    Pair<u64, u8> size_and_padding() { return { this->size(), this->padding() }; }

    void set_size(u64 new_size) { this->bits.set_mask(new_size, BIT_MASK_64_HIGH_56); }
    void set_padding(u8 new_padding) { this->bits.set_mask(u64(new_padding), BIT_MASK_64_LOW_8); };
    void set_size_and_padding(u64 size, u8 padding)
    {
        this->set_size(size);
        this->set_padding(padding);
    }
};

struct Arena final : Allocator
{
    static constexpr sz DEFAULT_CAPACITY = 4096;
    static constexpr sz MAX_ALLOCATION_SIZE = (sz)BIT_MASK_64_LOW_56;
 
    sz capacity;
    sz cursor;
    sz mark_count;
    Allocator* backing_alloc;
    FallbackAllocation* fallback_root;

    static Arena* create(Allocator* backing_alloc, sz init_capacity = Arena::DEFAULT_CAPACITY);
    void destroy();
    sz save_mark();
    void restore_mark(sz mark);
    bool is_last_alloc(sz cursor, void* ptr, sz alloc_size);
    void* fallback_allocate(sz size, sz alignment = 0, bool zero_mem = 0);
    void fallback_free_all();

    u8* mem_begin() { return (u8*)this + sizeof(Arena); }
    u8* mem_end() { return this->mem_begin() + this->capacity; }
    sz remain_mem() { return this->capacity - this->cursor; }
    u8* cursor_ptr() { return this->mem_begin() + this->cursor; }
    bool owns_ptr(void* ptr) { return ptr >= this->mem_begin() && ptr <= this->cursor_ptr(); }
};

void* arena_allocate(Allocator* self, sz size, sz alignment = 0, bool zero_mem = false);
void* arena_reallocate(Allocator* self, void* ptr, sz new_size, sz alignment = 0);
bool arena_resize(Allocator* self, void* ptr, sz new_size, sz alignment = 0);
void arena_free(Allocator* self, void* ptr);
void arena_display_info(Allocator* self);

// Thread safe version of arena.
// use case - shared, fast to access, storage.
struct ThreadArena final : Allocator
{
    static constexpr sz DEFAULT_CAPACITY = 4096;
    static constexpr sz MAX_ALLOCATION_SIZE = (sz)BIT_MASK_64_LOW_56;
 
    sz capacity;
    sz cursor;
    Atomic<sz> mark_count;
    Allocator* backing_alloc;
    FallbackAllocation* fallback_root;
    Mutex mutex;

    static ThreadArena* create(Allocator* backing_alloc, sz init_capacity = Arena::DEFAULT_CAPACITY);
    void destroy();
    sz save_mark();
    void restore_mark(sz mark);
    bool is_last_alloc(sz cursor, void* ptr, sz alloc_size);
    void* fallback_allocate(sz size, sz alignment = 0, bool zero_mem = 0);
    void fallback_free_all();

    u8* mem_begin() { return (u8*)this + sizeof(*this); }
    u8* mem_end() { return this->mem_begin() + this->capacity; }
    sz remain_mem() { return this->capacity - this->cursor; }
    u8* cursor_ptr() { return this->mem_begin() + this->cursor; }
    bool owns_ptr(void* ptr) { return ptr >= this->mem_begin() && ptr <= this->cursor_ptr(); }
};

void* thread_arena_allocate(Allocator* self, sz size, sz alignment = 0, bool zero_mem = false);
void* thread_arena_reallocate(Allocator* self, void* ptr, sz new_size, sz alignment = 0);
bool thread_arena_resize(Allocator* self, void* ptr, sz new_size, sz alignment = 0);
void thread_arena_free(Allocator* self, void* ptr);
void thread_arena_display_info(Allocator* self);

/*
 Pool allocator.
 This one isn't connected to 'Allocator' interface, because its specific.
 It can only allocate memory of a certain size or Type.
*/

// Tracks free nodes with bitset,
// compared to having a free list, it has a benefit of not using space inside the nodes,
// so, space consumption is reduced drastically.
// And we also can forget about manipulating links between doubly-linked list.
struct PoolAllocator
{
    static constexpr sz DEFAULT_NODE_COUNT = 128;

    sz node_size;
    sz node_count;
    Allocator* backing_alloc; 
    // bit == one -> node used, bit == zero -> node is free.
    DBitSet<u64> bitset;

    static PoolAllocator* create(Allocator* backing_alloc, sz node_size, sz node_alignment, sz node_count = DEFAULT_NODE_COUNT);
    void* allocate();
    void free(void* ptr);
    Maybe<u8*> get_first_available_node();
    // Doesn't check if its full.
    u8* get_first_available_node_dont_check();
    u8* get_node_by_idx(sz idx);
    void destroy();

    void reset() { this->bitset.clear(); }
    sz capacity() { return this->node_size * this->node_count; }
    u8* begin() { return (u8*)this + sizeof(*this); }
    u8* end() { return this->begin() + this->capacity(); }
    bool owns_ptr(void* ptr) { return ptr >= this->begin() && ptr < this->end(); }
};

} // rg

#endif // _RG_ALLOCATORS_HPP_
