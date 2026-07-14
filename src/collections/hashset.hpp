#ifndef _RG_HASHSET_HPP_
#define _RG_HASHSET_HPP_

#include "core/basic.hpp"
#include "collections/slice.hpp"

// HashSet.
// Enforces capacity of power of 2's for more efficient index calc.
// Values must be 'hashable' (have method .hash()) on them.
// Wrap primitives with Hashable type.
// Soa version.
// Data layout: [hashes...values].

namespace rg
{

intern constexpr u64 HASH_EMPTY = 0;
intern constexpr u64 HASH_DELETED = 1;
intern constexpr u64 HASH_FIRST_VALID = 2;

template<typename Type>
intern u64 calc_hash(Type key);
intern inline sz get_idx_for_hash(u64 hash, sz capacity);

template<typename Type>
struct HashSet
{
    static constexpr sz DEFAULT_CAPACITY = 16;
    static constexpr f32 DEFAULT_LOAD_FACTOR = 0.85;

    struct Iter
    {
        u64* data;
        sz capacity;
        sz pos;

        inline bool is_at_end() { return this->pos >= this->capacity; };
        inline void reset() { this->pos = 0; }
        Type* next_value();
    };

    u64* data;
    Allocator* alloc;
    sz count;
    sz capacity;
    f32 load_factor;

    HashSet();
    HashSet(HashSet&& rhs);
    HashSet& operator=(HashSet&& rhs);
    HashSet(const HashSet& rhs) = delete;
    HashSet& operator=(const HashSet& rhs) = delete;

    void init(Allocator* alloc, sz init_capacity = DEFAULT_CAPACITY, f32 load_factor = DEFAULT_LOAD_FACTOR);
    void init_with_values(Allocator* alloc, Slice<Type> values, sz add_capacity = 0, f32 load_factor = DEFAULT_LOAD_FACTOR);
    void put(const Type& val);
    bool has(const Type& key);
    Type* get(const Type& key);
    bool remove(const Type& key);
    void destroy();
    Iter get_iter();
    void foreach_value(void(*fn)(const Type&));
    void foreach_value_mut(void(*fn)(Type*));
    HashSet<Type> clone();

    sz len() { return this->count; }
    bool is_empty() { return this->count == 0; }
    bool is_initialized() { return this->alloc != null; }
    sz byte_size_used() { return this->count * (sizeof(u64) + sizeof(Type) + sizeof(Type)); }
    sz byte_size_allocated() { return this->capacity * (sizeof(u64) + sizeof(Type) + sizeof(Type)); }
private:
    void clear(Slice<u64> hashes);
    sz find_idx(const Type& key, Type* keys, Type* values);
    static void put_inner(u64* data, sz capacity, sz* count, u64 hash, const Type& val);
    static inline Type* values_begin(u64* data, sz capacity);
    static inline sz calc_mem_req(sz want_items);
    bool crossed_threshold();
    void grow();
};

template<typename Type>
HashSet<Type>::HashSet()
    : data{null}, alloc{null}, count{0}, capacity{0}, load_factor{0}
{}

template<typename Type>
HashSet<Type>::HashSet(HashSet&& rhs)
    : data{rhs.data}, alloc{rhs.alloc}, count{rhs.count}, capacity{rhs.capacity}, load_factor{rhs.load_factor}
{
    rhs.data = null;
    rhs.alloc = null;
    rhs.count = 0;
    rhs.capacity = 0;
    rhs.load_factor = 0;
}

template<typename Type>
HashSet<Type>& HashSet<Type>::operator=(HashSet&& rhs)
{
    if (this == &rhs) return *this;

    this->data = rhs.data;
    this->alloc = rhs.alloc;
    this->count = rhs.count;
    this->capacity = rhs.capacity;
    this->load_factor = rhs.load_factor;

    rhs.data = null;
    rhs.alloc = null;
    rhs.count = 0;
    rhs.capacity = 0;
    rhs.load_factor = 0;
    return *this;
}

template<typename Type>
void HashSet<Type>::init(Allocator* alloc, sz capacity, f32 load_factor)
{
    capacity = next_power_of_2(max(DEFAULT_CAPACITY, capacity));
    sz mem_req = calc_mem_req(capacity); 
    this->data = (u64*)allocator_allocate(alloc, mem_req);
    this->alloc = alloc;
    this->capacity = capacity;
    this->count = 0;
    this->load_factor = load_factor;
    this->clear({ this->data, this->capacity });
}

template<typename Type>
void HashSet<Type>::init_with_values(Allocator* alloc, Slice<Type> pairs, sz add_capacity, f32 load_factor)
{
    sz capacity = next_power_of_2(max(DEFAULT_CAPACITY, pairs.count + add_capacity));
    sz mem_req = calc_mem_req(capacity); 
    this->data = (u64*)allocator_allocate(alloc, mem_req);
    this->alloc = alloc;
    this->capacity = capacity;
    this->load_factor = load_factor;
    this->count = 0;
    this->clear({ this->data, this->capacity });

    for (auto* curr = pairs.begin(); curr != pairs.end(); ++curr)
    {
        this->put(curr->key, curr->val);
    }
}

template<typename Type>
void HashSet<Type>::clear(Slice<u64> hashes)
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized");
    u64* curr = hashes.ptr;
    u64* end = hashes.end();
    for (; curr != end; ++curr)
    {
        *curr = HASH_EMPTY;
    }
}

template<typename Type>
void HashSet<Type>::put(const Type& val)
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized first");
    if (this->crossed_threshold()) this->grow();
    u64 hash = calc_hash(val);
    put_inner(this->data, this->capacity, &this->count, hash, val);
}

template<typename Type>
void HashSet<Type>::put_inner(u64* data, sz capacity, sz* count, u64 hash, const Type& val)
{
    sz idx = get_idx_for_hash(hash, capacity);
    sz curr_hash_idx = idx;
    Type* values = values_begin(data, capacity);

    for (; curr_hash_idx < capacity; ++curr_hash_idx)
    {
        if (data[curr_hash_idx] < HASH_FIRST_VALID)
        {
            data[curr_hash_idx] = hash;
            values[curr_hash_idx] = val;
            if (count) (*count)++;
            return;
        }
    }

    // Wrapped around, start from first entry.
    curr_hash_idx = 0;
 
    for (; curr_hash_idx < idx; ++curr_hash_idx)
    {
        if (data[curr_hash_idx] < HASH_FIRST_VALID)
        {
            data[curr_hash_idx] = hash;
            values[curr_hash_idx] = val;
            if (count) (*count)++;
            return;
        }
    }

    ASSERT_MSG(false, "Failed to insert a value into a hashmap");
}

template<typename Type>
Type* HashSet<Type>::get(const Type& val)
{
    u64 search_hash = calc_hash(val);
    sz idx = get_idx_for_hash(search_hash, this->capacity);
    Type* values = values_begin(this->data, this->capacity);
    sz curr_idx = idx;

    for (; curr_idx != this->capacity; ++curr_idx)
    {
        if (this->data[curr_idx] == HASH_EMPTY) return null;
        if (this->data[curr_idx] == search_hash && values[curr_idx] == val) return values + curr_idx; 
    }

    // if we searched from the start, no need to do wrap around search.
    if (idx == 0) return null;

    curr_idx = 0;

    for (; curr_idx < idx; ++curr_idx)
    {
        if (this->data[curr_idx] == HASH_EMPTY) return null;
        if (this->data[curr_idx] == search_hash && values[curr_idx] == val) return values + curr_idx; 
    }

    return null;
}

template<typename Type>
bool HashSet<Type>::has(const Type& search)
{
    Type* value = this->get(search);
    return value != null;
}

template<typename Type>
bool HashSet<Type>::remove(const Type& val)
{
    u64 search_hash = calc_hash(val);
    sz idx = get_idx_for_hash(search_hash, this->capacity);
    Type* values = values_begin(this->data, this->capacity);
    sz curr_idx = idx;

    for (; curr_idx < this->capacity; ++curr_idx)
    {
        if (this->data[curr_idx] == HASH_EMPTY) return false;
        if (this->data[curr_idx] == search_hash && values[curr_idx] == val)
        {
            this->data[curr_idx] = HASH_DELETED;
            this->count--;
            return true;
        }
    }

    // if we searched from the start, no need to do wrap around search.
    if (idx == 0) return false;

    curr_idx = 0;

    for (; curr_idx < this->capacity; ++curr_idx)
    {
        if (this->data[curr_idx] == HASH_EMPTY) return false;
        if (this->data[curr_idx] == search_hash && values[curr_idx] == val)
        {
            this->data[curr_idx] = HASH_DELETED;
            this->count--;
            return true;
        }
    }

    return false;
}


template<typename Type>
void HashSet<Type>::grow()
{
    u64* old_data = this->data;
    sz old_capacity = this->capacity;
    sz new_capacity = max(old_capacity, DEFAULT_CAPACITY) * 2;
    sz mem_req = calc_mem_req(new_capacity);
    u64* new_data = (u64*)allocator_allocate(this->alloc, mem_req);
    this->clear({ new_data, new_capacity });

    sz old_idx = 0;
    auto old_values = values_begin(old_data, old_capacity);

    for (; old_idx < old_capacity; ++old_idx)
    {
        if (old_data[old_idx] < HASH_FIRST_VALID) continue;
        // Re-index and insert.
        const Type& old_val = old_values[old_idx];
        u64 hash = old_data[old_idx];
        put_inner(new_data, new_capacity, null, hash, old_val);
    }

    allocator_free(this->alloc, old_data);
    this->data = new_data;
    this->capacity = new_capacity;
}

template<typename Type>
HashSet<Type> HashSet<Type>::clone()
{
    ASSERT_INITIALIZED(this);

    HashSet<Type> res;
    sz mem_req = calc_mem_req(this->capacity);
    res.data = (u64*)allocator_allocate(this->alloc, mem_req);
    mem_copy(res.data, this->data, mem_req);
    res.alloc = this->alloc;
    res.count = this->count;
    res.capacity = this->capacity;
    return res;
}

template<typename Type>
void HashSet<Type>::destroy()
{
    if (this->data)
    {
        allocator_free(this->alloc, this->data);
        this->count = 0;
        this->capacity = 0;
        this->data = null;
    }
}

template<typename Type>
bool HashSet<Type>::crossed_threshold()
{
    f32 ratio = this->count / f32(this->capacity);
    return ratio > this->load_factor;
}

template<typename Type>
inline sz HashSet<Type>::calc_mem_req(sz want_items)
{
    return sizeof(u64) * want_items + sizeof(Type) * want_items + alignof(Type);
}

template<typename Type>
inline Type* HashSet<Type>::values_begin(u64* data, sz capacity)
{
    Type* res = align_ptr((Type*)(data + capacity));
    return res;
}

template<typename Type>
inline HashSet<Type>::Iter HashSet<Type>::get_iter()
{
    HashSet<Type>::Iter res;
    res.data = this->data;
    res.capacity = this->capacity;
    res.pos = 0;
    return res;
}

template<typename Type>
void HashSet<Type>::foreach_value(void(*fn)(const Type&))
{
    auto iter = this->get_iter();
    Type* value_ref;

    for (;;)
    {
        value_ref = iter.next_value();
        if (!value_ref) return;
        fn(value_ref);
    }
}

template<typename Type>
void HashSet<Type>::foreach_value_mut(void(*fn)(Type*))
{
    auto iter = this->get_iter();
    Type* value_ref;

    for (;;)
    {
        value_ref = iter.next_value();
        if (!value_ref) return;
        fn(value_ref);
    }
}

// Iterator.

template<typename Type>
Type* HashSet<Type>::Iter::next_value()
{
    if (this->is_at_end()) return null;
    auto values = values_begin(this->data, this->capacity);
    return values + this->pos;
}

// Shared.

template<typename Type>
intern u64 calc_hash(Type value)
{
    return rg::max(value.hash(), HASH_FIRST_VALID);
}

intern inline sz get_idx_for_hash(u64 hash, sz capacity)
{
    u64 cap_u = u64(capacity);
    ASSERT_POW_OF_TWO(cap_u);
    return max(sz(hash & cap_u - 1u) - sz(HASH_FIRST_VALID), 0l);
}

} // rg

#endif // _RG_HASHSET_HPP_
