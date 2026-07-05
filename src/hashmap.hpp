#ifndef _RG_HASHMAP_HPP_
#define _RG_HASHMAP_HPP_

#include "basic.hpp"
#include "slice.hpp"

// Hashmap.
// Enforces capacity of power of 2's for more efficient index calc.

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
    constexpr sz byte_count = sizeof(Type);
    char* byte = (char*)this;
    char* end = byte + byte_count;

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

template<typename Key, typename Value>
struct HashmapEntry
{
    Key key;
    Value val;
    u64 hash;
};

template<typename Key, typename Value>
struct HashmapIter
{
    Slice<HashmapEntry<Key, Value>> view;
    sz pos;

    HashmapEntry<Key, Value>* next_entry();
    Key* next_key();
    Value* next_value();
private:
    inline HashmapEntry<Key, Value>* curr_entry();
    inline bool is_at_end();
};

constexpr sz HASHMAP_DEFAULT_CAPACITY = 16;
constexpr f32 HASHMAP_DEFAULT_LOAD_FACTOR = 0.85;
intern constexpr u64 HASH_EMPTY = 0;
intern constexpr u64 HASH_DELETED = 1;
intern constexpr u64 HASH_FIRST_VALID = 2;

template<typename Key, typename Value>
struct Hashmap
{
    HashmapEntry<Key, Value>* data;
    Allocator* alloc;
    sz count;
    sz capacity;
    f32 load_factor;

    // Constructor is just to detect initialized state.
    Hashmap();
    void init(Allocator* alloc, sz init_capacity = HASHMAP_DEFAULT_CAPACITY, f32 load_factor = HASHMAP_DEFAULT_LOAD_FACTOR);
    void init_with_key_values(Allocator* alloc, Slice<HashmapKVPair<Key, Value>> pairs, sz init_capacity = HASHMAP_DEFAULT_CAPACITY, f32 load_factor = HASHMAP_DEFAULT_LOAD_FACTOR);
    void put(Key key, Value val);
    bool has(Key key);
    Value* get(Key key);
    bool remove(Key key);
    void clear(Slice<HashmapEntry<Key, Value>> entries);
    void destroy();
    HashmapIter<Key, Value> get_iter();
    void foreach_entry(void(*fn)(HashmapEntry<Key, Value>));
    void foreach_entry_ref(void(*fn)(HashmapEntry<Key, Value>*));
    void foreach_key(void(*fn)(Key));
    void foreach_key_ref(void(*fn)(Key*));
    void foreach_value(void(*fn)(Value));
    void foreach_value_ref(void(*fn)(Value*));

    inline sz len() { return this->count; }
    inline bool is_empty() { return this->count == 0; }
    inline bool is_initialized() { return this->data != null && this->alloc != null; }
    inline sz byte_size_used() { return this->count * sizeof(HashmapEntry<Key, Value>); }
    inline sz byte_size_allocated() { return this->capacity * sizeof(HashmapEntry<Key, Value>); }
private:
    inline HashmapEntry<Key, Value> at(sz idx) { return *(this->data + idx); }
    inline HashmapEntry<Key, Value>* at_ref(sz idx) { return this->data + idx; }
    inline sz remain() { return this->capacity - this->count; }
    bool crossed_threshold();
    void grow();
};

template<typename Key>
intern u64 calc_hash(Key key);
intern sz get_idx_for_hash(u64 hash, sz capacity);

template<typename Key, typename Value>
Hashmap<Key, Value>::Hashmap()
    : data{null}, alloc{null} 
{
}

template<typename Key, typename Value>
void Hashmap<Key, Value>::init(Allocator* alloc, sz init_capacity, f32 load_factor)
{
    ASSERT_MSG(load_factor > 0, "Must be bigger than 0");
    init_capacity = next_power_of_2(max(HASHMAP_DEFAULT_CAPACITY, init_capacity));
    this->data = (HashmapEntry<Key, Value>*)allocator_allocate(alloc, init_capacity * sizeof(HashmapEntry<Key, Value>));
    this->capacity = init_capacity;
    this->alloc = alloc;
    this->load_factor = load_factor;
    this->clear({ this->data, this->capacity });
}

template<typename Key, typename Value>
void Hashmap<Key, Value>::init_with_key_values(Allocator* alloc, Slice<HashmapKVPair<Key, Value>> pairs, sz init_capacity, f32 load_factor)
{
    ASSERT_MSG(load_factor > 0, "Must be bigger than 0");
    init_capacity = next_power_of_2(max(HASHMAP_DEFAULT_CAPACITY, pairs.count, init_capacity));
    this->data = (HashmapEntry<Key, Value>*)allocator_allocate(alloc, init_capacity * sizeof(HashmapEntry<Key, Value>));
    this->capacity = init_capacity;
    this->alloc = alloc;
    this->load_factor = load_factor;
    this->clear({ this->data, this->capacity });

    for (auto& pair : pairs)
    {
        this->put(pair.key, pair.val);
    }
}

template<typename Key, typename Value>
void Hashmap<Key, Value>::clear(Slice<HashmapEntry<Key, Value>> entries)
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized");
    this->count = 0;

    HashmapEntry<Key, Value>* curr = entries.ptr;
    HashmapEntry<Key, Value>* end = curr + entries.count;
    for (; curr != end; ++curr)
    {
        curr->hash = HASH_EMPTY;
    }
}

template<typename Key, typename Value>
void Hashmap<Key, Value>::put(Key key, Value val)
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized first");

    if (this->crossed_threshold()) this->grow();

    u64 hash = calc_hash(key);
    sz idx = get_idx_for_hash(hash, this->capacity);

    HashmapEntry<Key, Value>* start = this->at_ref(idx);
    HashmapEntry<Key, Value>* entry = start;
    HashmapEntry<Key, Value>* end = this->data + this->capacity;

    for (; entry != end; ++entry)
    {
        if (entry->hash < HASH_FIRST_VALID)
        {
            entry->key = key;
            entry->val = val;
            entry->hash = hash;
            return;
        }
    }

    // Wrapped around, start from first entry.
    entry = this->data;
    end = start;
    
    for (; entry != end; ++entry)
    {
        if (entry->hash < HASH_FIRST_VALID)
        {
            entry->key = key;
            entry->val = val;
            entry->hash = hash;
            return;
        }
    }

    ASSERT_MSG(false, "Failed to insert a value into a hashmap");
}

template<typename Key, typename Value>
Value* Hashmap<Key, Value>::get(Key key)
{
    u64 search_hash = calc_hash(key);
    sz idx = get_idx_for_hash(search_hash, this->capacity);

    HashmapEntry<Key, Value>* start = this->at_ref(idx);
    HashmapEntry<Key, Value>* entry = start;
    HashmapEntry<Key, Value>* end = this->data + this->capacity;

    for (; entry != end; ++entry)
    {
        if (entry->hash == search_hash && entry->key == key) return &entry->val; 
    }

    // if we searched from the start, no need to do wrap around search.
    if (idx == HASH_FIRST_VALID) return null;

    entry = this->data;
    end = start;

    for (; entry != end; ++entry)
    {
        if (entry->hash == search_hash && entry->key == key) return &entry->val;
    }

    return null;
}

template<typename Key, typename Value>
bool Hashmap<Key, Value>::has(Key key)
{
    Value* val = this->get(key);
    return val != null;
}

template<typename Key, typename Value>
bool Hashmap<Key, Value>::remove(Key key)
{
    u64 search_hash = calc_hash(key);
    sz idx = get_idx_for_hash(search_hash, this->capacity);

    HashmapEntry<Key, Value>* start = this->at_ref(idx);
    HashmapEntry<Key, Value>* entry = start;
    HashmapEntry<Key, Value>* end = this->data + this->capacity;

    for (; entry != end; ++entry)
    {
        if (entry->hash == search_hash && entry->key == key)
        {
            entry->hash = HASH_DELETED;
            return true;
        } 
    }

    // if we searched from the start, no need to do wrap around search.
    if (idx == HASH_FIRST_VALID) return false;

    entry = this->data;
    end = start;

    for (; entry != end; ++entry)
    {
        if (entry->hash == search_hash && entry->key == key)
        {
            entry->hash = HASH_DELETED;
            return true;
        } 
    }

    return false;
}

template<typename Key, typename Value>
void Hashmap<Key, Value>::grow()
{
    sz new_capacity = max(this->capacity, HASHMAP_DEFAULT_CAPACITY) * 2;
    HashmapEntry<Key, Value>* new_entries = (HashmapEntry<Key, Value>*)allocator_allocate(this->alloc, new_capacity * sizeof(HashmapEntry<Key, Value>));
    this->clear({ new_entries, new_capacity });

    HashmapEntry<Key, Value>* curr_entry = this->data;
    HashmapEntry<Key, Value>* curr_end = this->data + this->capacity;

    for (; curr_entry != curr_end; ++curr_entry)
    {
        if (curr_entry->hash < HASH_FIRST_VALID) continue;
        // Rehash and insert.
        u64 hash = calc_hash(curr_entry->key);
        sz new_idx = get_idx_for_hash(hash, new_capacity);
        new_entries[new_idx] = *curr_entry;
    }

    allocator_free(this->alloc, this->data);
    this->data = new_entries;
    this->capacity = new_capacity;
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

template<typename Key>
intern u64 calc_hash(Key key)
{
    return max(key.hash(), HASH_FIRST_VALID);
}

intern inline sz get_idx_for_hash(u64 hash, sz capacity)
{
    return sz(hash & u64(capacity - 1));
}

// HashmapIter.

template<typename Key, typename Value>
HashmapIter<Key, Value> Hashmap<Key, Value>::get_iter()
{
    HashmapIter<Key, Value> iter;
    iter.view = { this->data, this->capacity };
    iter.pos = 0;
    return iter;
}

template<typename Key, typename Value>
void Hashmap<Key, Value>::foreach_entry(void(*fn)(HashmapEntry<Key, Value>))
{
    HashmapIter<Key, Value> iter = this->get_iter();
    HashmapEntry<Key, Value>* curr_entry;
    
    for (;;)
    {
        curr_entry = iter.next_entry();
        if (!curr_entry) break;
        fn(*curr_entry);
    }
}

template<typename Key, typename Value>
void Hashmap<Key, Value>::foreach_entry_ref(void(*fn)(HashmapEntry<Key, Value>*))
{
    HashmapIter<Key, Value> iter = this->get_iter();
    HashmapEntry<Key, Value>* curr_entry;
    
    for (;;)
    {
        curr_entry = iter.next_entry();
        if (!curr_entry) break;
        fn(curr_entry);
    }
}

template<typename Key, typename Value>
void Hashmap<Key, Value>::foreach_key(void(*fn)(Key))
{
    HashmapIter<Key, Value> iter = this->get_iter();
    Key* curr_key;
    
    for (;;)
    {
        curr_key = iter.next_key();
        if (!curr_key) break;
        fn(*curr_key);
    }
}

template<typename Key, typename Value>
void Hashmap<Key, Value>::foreach_key_ref(void(*fn)(Key*))
{
    HashmapIter<Key, Value> iter = this->get_iter();
    Key* curr_key;
    
    for (;;)
    {
        curr_key = iter.next_key();
        if (!curr_key) break;
        fn(curr_key);
    }
}

template<typename Key, typename Value>
void Hashmap<Key, Value>::foreach_value(void(*fn)(Value))
{
    HashmapIter<Key, Value> iter = this->get_iter();
    Value* curr_val;
    
    for (;;)
    {
        curr_val = iter.next_value();
        if (!curr_val) break;
        fn(*curr_val);
    }
}

template<typename Key, typename Value>
void Hashmap<Key, Value>::foreach_value_ref(void(*fn)(Value*))
{
    HashmapIter<Key, Value> iter = this->get_iter();
    Value* curr_val;
    
    for (;;)
    {
        curr_val = iter.next_value();
        if (!curr_val) break;
        fn(curr_val);
    }
}

template<typename Key, typename Value>
HashmapEntry<Key, Value>* HashmapIter<Key, Value>::next_entry()
{
    if (this->is_at_end()) return null;
    HashmapEntry<Key, Value>* start = this->curr_entry();
    HashmapEntry<Key, Value>* entry = start;
    HashmapEntry<Key, Value>* end = this->view.end();

    for (; entry != end; ++entry)
    {
        if (entry->hash >= HASH_FIRST_VALID)
        {
            this->pos += (entry - start) + 1;
            return entry;
        }
    }

    this->pos = this->view.count;
    return null;
}

template<typename Key, typename Value>
Key* HashmapIter<Key, Value>::next_key()
{
    HashmapEntry<Key, Value>* entry = this->next_entry();
    if (!entry) return null;
    return &entry->key;
}

template<typename Key, typename Value>
Value* HashmapIter<Key, Value>::next_value()
{
    HashmapEntry<Key, Value>* entry = this->next_entry();
    if (!entry) return null;
    return &entry->val;
}

template<typename Key, typename Value>
inline bool HashmapIter<Key, Value>::is_at_end()
{
    return this->pos >= this->view.count;
}

template<typename Key, typename Value>
inline HashmapEntry<Key, Value>* HashmapIter<Key, Value>::curr_entry()
{
    return this->view.ptr + this->pos;
}

} // rg

#endif // _RG_HASHMAP_HPP_
