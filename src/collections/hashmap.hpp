#ifndef _RG_HASHMAP_HPP_
#define _RG_HASHMAP_HPP_

#include <emmintrin.h>
#include <immintrin.h>
#include "core/basic.hpp"
#include "collections/slice.hpp"

// Hashmap.
// Enforces capacity of power of 2's for more efficient index calc.
// Soa version.
// Data layout: [hashes...keys...values].

namespace rg
{

// Should only be used for wrappping primitive types.
// Other types of keys should implement 'hash' method.
template<typename Type>
struct Hashable
{
    Type val;
    u64 hash();
};

template<typename Type>
u64 Hashable<Type>::hash()
{
    u64 hash = FNV_OFFSET_BASIS;
    constexpr sz BYTE_COUNT = sizeof(Type);
    char* byte = (char*)this;
    char* end = byte + BYTE_COUNT;

    for (; byte != end; ++byte)
    {
        hash ^= *byte;
        hash *= FNV_PRIME;
    }

    return hash;
}

template<typename Key, typename Value>
struct HashmapKVPair
{
    Key key;
    Value val;
};

intern constexpr u64 HASH_EMPTY = 0;
intern constexpr u64 HASH_DELETED = 1;
intern constexpr u64 HASH_FIRST_VALID = 2;

template<typename Key>
intern u64 calc_hash(Key key);
intern inline sz get_idx_for_hash(u64 hash, sz capacity);

template<typename Key, typename Value>
struct Hashmap
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
        Pair<Key*, Value*> next_pair();
        Key* next_key();
        Value* next_value();
    };

    u64* data;
    Allocator* alloc;
    sz count;
    sz capacity;
    f32 load_factor;

    Hashmap();
    Hashmap(Hashmap&& rhs);
    Hashmap& operator=(Hashmap&& rhs);
    Hashmap(const Hashmap& rhs) = delete;
    Hashmap& operator=(const Hashmap& rhs) = delete;

    void init(Allocator* alloc, sz init_capacity = DEFAULT_CAPACITY, f32 load_factor = DEFAULT_LOAD_FACTOR);
    void init_with_key_values(Allocator* alloc, Slice<HashmapKVPair<Key, Value>> pairs, sz add_capacity = 0, f32 load_factor = DEFAULT_LOAD_FACTOR);
    void put(const Key& key, const Value& val);
    bool has(const Key& key);
    Value* get(const Key& key);
    bool remove(const Key& key);
    void destroy();
    Iter get_iter();
    void foreach_pair(void(*fn)(const Pair<Key*, Value*>&));
    void foreach_pair_mut(void(*fn)(Pair<Key*, Value*>));
    void foreach_key(void(*fn)(const Key&));
    void foreach_key_mut(void(*fn)(Key*));
    void foreach_value(void(*fn)(const Value&));
    void foreach_value_mut(void(*fn)(Value*));
    Hashmap<Key, Value> clone();

    inline sz len() { return this->count; }
    inline bool is_empty() { return this->count == 0; }
    inline bool is_initialized() { return this->data != null && this->alloc != null; }
    inline sz byte_size_used() { return this->count * (sizeof(u64) + sizeof(Key) + sizeof(Value)); }
    inline sz byte_size_allocated() { return this->capacity * (sizeof(u64) + sizeof(Key) + sizeof(Value)); }
private:
    void clear(Slice<u64> hashes);
    sz find_idx(const Key& key, Key* keys, Value* values);
    static void put_inner(u64* data, sz capacity, sz* count, u64 hash, const Key& key, const Value& val);
    static inline Pair<Key*, Value*> keys_values_begin(u64* data, sz capacity);
    static inline sz calc_mem_req(sz want_items);
    bool crossed_threshold();
    void grow();
};

template<typename Key, typename Value>
Hashmap<Key, Value>::Hashmap()
    : data{null}, alloc{null}, count{0}, capacity{0}, load_factor{0}
{}

template<typename Key, typename Value>
Hashmap<Key, Value>::Hashmap(Hashmap&& rhs)
    : data{rhs.data}, alloc{rhs.alloc}, count{rhs.count}, capacity{rhs.capacity}, load_factor{rhs.load_factor}
{
    rhs.data = null;
    rhs.alloc = null;
    rhs.count = 0;
    rhs.capacity = 0;
    rhs.load_factor = 0;
}

template<typename Key, typename Value>
Hashmap<Key, Value>& Hashmap<Key, Value>::operator=(Hashmap&& rhs)
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

template<typename Key, typename Value>
void Hashmap<Key, Value>::init(Allocator* alloc, sz capacity, f32 load_factor)
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

template<typename Key, typename Value>
void Hashmap<Key, Value>::init_with_key_values(Allocator* alloc, Slice<HashmapKVPair<Key, Value>> pairs, sz add_capacity, f32 load_factor)
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

template<typename Key, typename Value>
void Hashmap<Key, Value>::clear(Slice<u64> hashes)
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized");
    u64* curr = hashes.ptr;
    u64* end = hashes.end();
    for (; curr != end; ++curr)
    {
        *curr = HASH_EMPTY;
    }
}

template<typename Key, typename Value>
void Hashmap<Key, Value>::put(const Key& key, const Value& val)
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized first");
    if (this->crossed_threshold()) this->grow();
    u64 hash = calc_hash(key);
    put_inner(this->data, this->capacity, &this->count, hash, key, val);
}

template<typename Key, typename Value>
void Hashmap<Key, Value>::put_inner(u64* data, sz capacity, sz* count, u64 hash, const Key& key, const Value& val)
{
    sz idx = get_idx_for_hash(hash, capacity);
    sz curr_hash_idx = idx;
    auto [keys, values] = keys_values_begin(data, capacity);

    for (; curr_hash_idx < capacity; ++curr_hash_idx)
    {
        if (data[curr_hash_idx] < HASH_FIRST_VALID)
        {
            data[curr_hash_idx] = hash;
            keys[curr_hash_idx] = key;
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
            keys[curr_hash_idx] = key;
            values[curr_hash_idx] = val;
            if (count) (*count)++;
            return;
        }
    }

    ASSERT_MSG(false, "Failed to insert a value into a hashmap");
}

template<typename Key, typename Value>
Value* Hashmap<Key, Value>::get(const Key& key)
{
    u64 search_hash = calc_hash(key);
    sz idx = get_idx_for_hash(search_hash, this->capacity);
    auto [keys, values] = keys_values_begin(this->data, this->capacity);
    sz curr_idx = idx;

    for (; curr_idx != this->capacity; ++curr_idx)
    {
        if (this->data[curr_idx] == HASH_EMPTY) return null;
        if (this->data[curr_idx] == search_hash && keys[curr_idx] == key) return values + curr_idx; 
    }

    // if we searched from the start, no need to do wrap around search.
    if (idx == 0) return null;

    curr_idx = 0;

    for (; curr_idx < idx; ++curr_idx)
    {
        if (this->data[curr_idx] == HASH_EMPTY) return null;
        if (this->data[curr_idx] == search_hash && keys[curr_idx] == key) return values + curr_idx; 
    }

    return null;
}

template<typename Key, typename Value>
bool Hashmap<Key, Value>::has(const Key& key)
{
    Value* val = this->get(key);
    return val != null;
}

template<typename Key, typename Value>
bool Hashmap<Key, Value>::remove(const Key& key)
{
    u64 search_hash = calc_hash(key);
    sz idx = get_idx_for_hash(search_hash, this->capacity);
    auto [keys, values] = keys_values_begin(this->data, this->capacity);
    sz curr_idx = idx;

    for (; curr_idx < this->capacity; ++curr_idx)
    {
        if (this->data[curr_idx] == HASH_EMPTY) return false;
        if (this->data[curr_idx] == search_hash && keys[curr_idx] == key)
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
        if (this->data[curr_idx] == search_hash && keys[curr_idx] == key)
        {
            this->data[curr_idx] = HASH_DELETED;
            this->count--;
            return true;
        }
    }

    return false;
}


template<typename Key, typename Value>
void Hashmap<Key, Value>::grow()
{
    u64* old_data = this->data;
    sz old_capacity = this->capacity;
    sz new_capacity = max(old_capacity, DEFAULT_CAPACITY) * 2;
    sz mem_req = calc_mem_req(new_capacity);
    u64* new_data = (u64*)allocator_allocate(this->alloc, mem_req);
    this->clear({ new_data, new_capacity });

    sz old_idx = 0;
    auto [old_keys, old_values] = keys_values_begin(old_data, old_capacity);

    for (; old_idx < old_capacity; ++old_idx)
    {
        if (old_data[old_idx] < HASH_FIRST_VALID) continue;
        // Re-index and insert.
        const Key& old_key = old_keys[old_idx];
        const Value& old_val = old_values[old_idx];
        u64 hash = old_data[old_idx];
        put_inner(new_data, new_capacity, null, hash, old_key, old_val);
    }

    allocator_free(this->alloc, old_data);
    this->data = new_data;
    this->capacity = new_capacity;
}

template<typename Key, typename Value>
Hashmap<Key, Value> Hashmap<Key, Value>::clone()
{
    ASSERT_INITIALIZED(this);

    Hashmap<Key, Value> res;
    sz mem_req = calc_mem_req(this->capacity);
    res.data = (u64*)allocator_allocate(this->alloc, mem_req);
    mem_copy(res.data, this->data, mem_req);
    res.alloc = this->alloc;
    res.count = this->count;
    res.capacity = this->capacity;
    return res;
}

template<typename Key, typename Value>
void Hashmap<Key, Value>::destroy()
{
    if (this->data)
    {
        allocator_free(this->alloc, this->data);
        this->count = 0;
        this->capacity = 0;
        this->data = null;
    }
}

template<typename Key, typename Value>
bool Hashmap<Key, Value>::crossed_threshold()
{
    f32 ratio = this->count / f32(this->capacity);
    return ratio > this->load_factor;
}

template<typename Key, typename Value>
inline sz Hashmap<Key, Value>::calc_mem_req(sz want_items)
{
    return sizeof(u64) * want_items + sizeof(Key) * want_items + sizeof(Value) * want_items + alignof(Key) + alignof(Value);
}

template<typename Key, typename Value>
inline Pair<Key*, Value*> Hashmap<Key, Value>::keys_values_begin(u64* data, sz capacity)
{
    Pair<Key*, Value*> res;
    res.first = align_ptr((Key*)(data + capacity));
    res.second = align_ptr((Value*)(res.first + capacity));
    return res;
}

template<typename Key, typename Value>
inline Hashmap<Key, Value>::Iter Hashmap<Key, Value>::get_iter()
{
    Hashmap<Key, Value>::Iter res;
    res.data = this->data;
    res.capacity = this->capacity;
    res.pos = 0;
    return res;
}

template<typename Key, typename Value>
void Hashmap<Key, Value>::foreach_pair(void(*fn)(const Pair<Key*, Value*>&))
{
    auto iter = this->get_iter();
    Pair<Key*, Value*> pair_ref;

    for (;;)
    {
        pair_ref = iter.next_pair();
        if (!pair_ref.first || !pair_ref.second) return;
        fn(*pair_ref);
    }
}

template<typename Key, typename Value>
void Hashmap<Key, Value>::foreach_pair_mut(void(*fn)(Pair<Key*, Value*>))
{
    auto iter = this->get_iter();
    Pair<Key*, Value*> pair_ref;

    for (;;)
    {
        pair_ref = iter.next_pair();
        if (!pair_ref.first || !pair_ref.second) return;
        fn(pair_ref);
    }
}

template<typename Key, typename Value>
void Hashmap<Key, Value>::foreach_key(void(*fn)(const Key&))
{
    auto iter = this->get_iter();
    Key* key_ref;

    for (;;)
    {
        key_ref = iter.next_key();
        if (!key_ref) return;
        fn(*key_ref);
    }
}

template<typename Key, typename Value>
void Hashmap<Key, Value>::foreach_key_mut(void(*fn)(Key*))
{
    auto iter = this->get_iter();
    Key* key_ref;

    for (;;)
    {
        key_ref = iter.next_key();
        if (!key_ref) return;
        fn(key_ref);
    }
}

template<typename Key, typename Value>
void Hashmap<Key, Value>::foreach_value(void(*fn)(const Value&))
{
    auto iter = this->get_iter();
    Value* value_ref;

    for (;;)
    {
        value_ref = iter.next_value();
        if (!value_ref) return;
        fn(value_ref);
    }
}

template<typename Key, typename Value>
void Hashmap<Key, Value>::foreach_value_mut(void(*fn)(Value*))
{
    auto iter = this->get_iter();
    Value* value_ref;

    for (;;)
    {
        value_ref = iter.next_value();
        if (!value_ref) return;
        fn(value_ref);
    }
}

// Iterator.

template<typename Key, typename Value>
Pair<Key*, Value*> Hashmap<Key, Value>::Iter::next_pair()
{
    if (this->is_at_end()) return { null, null };
    auto [keys, values] = keys_values_begin(this->data, this->capacity);
    return { keys + this->pos, values + this->pos };
}

template<typename Key, typename Value>
Key* Hashmap<Key, Value>::Iter::next_key()
{
    if (this->is_at_end()) return null;
    auto [keys, _] = keys_values_begin(this->data, this->capacity);
    return keys + this->pos;
}

template<typename Key, typename Value>
Value* Hashmap<Key, Value>::Iter::next_value()
{
    if (this->is_at_end()) return null;
    auto [_, values] = keys_values_begin(this->data, this->capacity);
    return values + this->pos;
}

// Shared.

template<typename Key>
intern u64 calc_hash(Key key)
{
    return max(key.hash(), HASH_FIRST_VALID);
}

intern inline sz get_idx_for_hash(u64 hash, sz capacity)
{
    u64 cap_u = u64(capacity);
    ASSERT_POW_OF_TWO(cap_u);
    return max(sz(hash & cap_u - 1u) - sz(HASH_FIRST_VALID), 0l);
}

} // rg

#endif // _RG_HASHMAP_HPP_
