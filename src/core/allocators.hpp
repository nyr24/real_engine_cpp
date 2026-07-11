#ifndef _RG_ALLOCATORS_HPP_
#define _RG_ALLOCATORS_HPP_

#include "core/basic.hpp"
#include "core/bits.hpp"

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
    Bits<u64> bits;
    VmemAllocHeader* prev_phys;

    sz size() { return (sz)this->bits.get(BIT_MASK_64_U56_HIGH); }
    bool is_free() { return (bool)this->bits.get(BIT_MASK_64_U8_LOW); }
    Pair<sz, bool> size_and_is_free()
    {
        return { this->size(), this->is_free() };
    }
    void set_size(sz new_size) { this->bits.set(u64(new_size), BIT_MASK_64_U56_HIGH); }
    void set_is_free(bool new_free) { this->bits.set(new_free, BIT_MASK_64_U8_LOW); } 
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
 Virtual memory allocator.
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
// Stateless, so can't handle alignment.

struct HeapAlloc final : Allocator
{
    void init();
};

void* heap_allocate(Allocator* self, sz size, sz alignment = 0, bool zero_mem = false);
void* heap_reallocate(Allocator* self, void* ptr, sz new_size, sz alignment = 0);
[[noreturn]] bool heap_resize(Allocator* self, void* ptr, sz new_size, sz alignment = 0);
void heap_free(Allocator* self, void* ptr);
void heap_display_info(Allocator* self);

// Arena allocator.

// Padding for simd types can be up to 64 bytes, so we must account for that.
// Layout: [7bytes=size..1byte=padding].
struct ArenaAllocHeader
{
private:
    Bits<u64, BitMask64> bits;
public:
    u64 size() { return this->bits.get(BIT_MASK_64_U56_HIGH); }
    u8 padding() { return (u8)this->bits.get(BIT_MASK_64_U16_LOW); };
    Pair<u64, u8> size_and_padding() { return { this->size(), this->padding() }; }

    void set_size(u64 new_size) { this->bits.set(new_size, BIT_MASK_64_U56_HIGH); }
    void set_padding(u8 new_padding) { this->bits.set(u64(new_padding), BIT_MASK_64_U16_LOW); };
    void set_size_and_padding(u64 size, u8 padding)
    {
        this->set_size(size);
        this->set_padding(padding);
    }
};

struct Arena final : Allocator
{
    static constexpr sz DEFAULT_CAPACITY = 4096;
    static constexpr sz MAX_ALLOCATION_SIZE = (sz)BIT_MASK_64_U56_HIGH;
 
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

// Tlsf.

/*
** Two Level Segregated Fit memory allocator, version 3.1.
** Written (originally) by Matthew Conte
**	http://tlsf.baisoku.org
**
** Based on the original documentation by Miguel Masmano:
**	http://www.gii.upv.es/tlsf/main/docs
**
** This implementation was written to the specification
** of the document, therefore no GPL restrictions apply.
** 
** Copyright (c) 2006-2016, Matthew Conte
** All rights reserved.
** 
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of the copyright holder nor the
**       names of its contributors may be used to endorse or promote products
**       derived from this software without specific prior written permission.
** 
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL MATTHEW CONTE BE LIABLE FOR ANY
** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <cstddef>

/* tlsf_t: a TLSF structure. Can contain 1 to N pools. */
/* pool_t: a block of memory that TLSF can manage. */
typedef void* tlsf_t;
typedef void* pool_t;

/* Create/destroy a memory pool. */
tlsf_t tlsf_create(void* mem);
tlsf_t tlsf_create_with_pool(void* mem, size_t bytes);
void tlsf_destroy(tlsf_t tlsf);
pool_t tlsf_get_pool(tlsf_t tlsf);

/* Add/remove memory pools. */
pool_t tlsf_add_pool(tlsf_t tlsf, void* mem, size_t bytes);
void tlsf_remove_pool(tlsf_t tlsf, pool_t pool);

/* malloc/memalign/realloc/free replacements. */
void* tlsf_malloc(tlsf_t tlsf, size_t bytes);
void* tlsf_memalign(tlsf_t tlsf, size_t align, size_t bytes);
void* tlsf_realloc(tlsf_t tlsf, void* ptr, size_t size);
void tlsf_free(tlsf_t tlsf, void* ptr);

/* Returns internal block size, not original request size */
size_t tlsf_block_size(void* ptr);

/* Overheads/limits of internal structures. */
size_t tlsf_size(void);
size_t tlsf_align_size(void);
size_t tlsf_block_size_min(void);
size_t tlsf_block_size_max(void);
size_t tlsf_pool_overhead(void);
size_t tlsf_alloc_overhead(void);

/* Debugging. */
typedef void (*tlsf_walker)(void* ptr, size_t size, int used, void* user);
void tlsf_walk_pool(pool_t pool, tlsf_walker walker, void* user);
/* Returns nonzero if any internal consistency check fails. */
int tlsf_check(tlsf_t tlsf);
int tlsf_check_pool(pool_t pool);

/*
 Pool allocator.
 This one isn't connected to 'Allocator' interface, because its specific.
 It can only allocate memory of a certain size or Type.
*/

// Node doesn't store its size, it would be wasteful.
// Its stored instide pool allocator structure.
struct PoolNode
{
    PoolNode* prev;
    PoolNode* next;
    // Data sits here.

    PoolNode* prev_phys(sz node_size) { return (PoolNode*)((u8*)this - sizeof(PoolNode) - node_size); }
    PoolNode* next_phys(sz node_size) { return (PoolNode*)(this->mem_begin() + node_size); }
    u8* mem_begin() { return (u8*)this + sizeof(*this); }
    static PoolNode* ptr_to_node(void* ptr) { return (PoolNode*)((u8*)ptr - sizeof(PoolNode)); }
};

struct PoolAllocator
{
    static constexpr sz DEFAULT_NODE_COUNT = 128;

    sz node_size;
    sz node_count;
    Allocator* backing_alloc; 
    // Freelist.
    PoolNode* free_root;

    static PoolAllocator* create(Allocator* backing_alloc, sz node_size, sz node_alignment, sz node_count = DEFAULT_NODE_COUNT);
    void* allocate();
    void free(void* ptr);
    void reset();
    void destroy();
    void* fallback_allocate(sz size, sz alignment, bool zero_mem);

    static sz calc_mem_req(sz node_size, sz node_count)
    {
        return (sizeof(PoolNode) + node_size) * node_count;
    }
    sz capacity() { return (this->node_size + sizeof(PoolNode)) * this->node_count; }
    PoolNode* begin() { return (PoolNode*)((u8*)this + sizeof(*this)); }
    PoolNode* end() { return (PoolNode*)(this->begin() + this->capacity()); }
    bool owns_ptr(void* ptr) { return ptr >= this->begin() && ptr < this->end(); }
};

} // rg

#endif // _RG_ALLOCATORS_HPP_
