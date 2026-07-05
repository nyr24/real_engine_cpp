#ifndef _RG_DARRAY_HPP_
#define _RG_DARRAY_HPP_

#include "basic.hpp"
#include "slice.hpp"
#include "split_iterator.hpp"

// Darray - dynamic array type.

namespace rg
{

constexpr sz DARRAY_DEFAULT_CAPACITY = 16;

template<typename Type>
struct DArray
{
    Type* data;
    Allocator* alloc;
    sz count;
    sz capacity;

    // Constructor is just to detect initialized state.
    DArray();
    void init(Allocator* alloc, sz init_capacity = DARRAY_DEFAULT_CAPACITY);
    void init_slice(Allocator* alloc, Slice<Type> values, sz additional_capacity = 0);
    void push(Type value);
    void push(Slice<Type> values);
    Type pop();
    Type remove_unordered_at(sz idx);
    void reserve(sz needed);
    void destroy();
    void move_ownership(DArray<Type>* other);
    Slice<Type> slice(sz start = 0, sz end = -1);
    // Trims 'count' characters from end.
    void trim_end_n(sz count);
    // Trims any value from the set. (input isn't considered sequential)
    void trim_value_set_end(Slice<Type> value_set);
    // Trims sequentially values from the end of input. (input is considered sequential)
    bool trim_sequence_end(Slice<Type> seq);
    void trim_from_end_to_first_occur(Type search, bool inclusive = false);
    void trim_from_end_to_last_occur(Type search, bool inclusive = false);
    bool starts_with(Slice<Type> input) const;
    bool ends_with(Slice<Type> input) const;
    sz index_of(Type val) const;
    sz index_of(Slice<Type> slice) const;
    sz last_index_of(Type val) const;
    sz last_index_of(Slice<Type> slice) const;
    bool has(Type val) const;
    bool has(Slice<Type> val) const;
    void replace(Type find, Type replace);
    void foreach(void(*fn)(Type));
    void foreach_ref(void(*fn)(Type*));
    SplitIterator<Type> get_split_iter(Type splitter);
    void foreach_split(Type splitter, void(*fn)(Slice<Type>));

    inline Type at(sz idx) const;
    inline Type* at_ref(sz idx);
    inline void set(Type val, sz idx);
    inline void swap(sz idx1, sz idx2);
    inline Type operator[](sz idx) const;

    inline sz len() const { return this->count; }
    inline Type* begin() { return this->data; }
    inline Type* end() { return this->data + this->count; }
    inline Type first() const { return *this->data; }
    inline Type* first_ref() { return this->data; }
    inline Type last() const { return *(this->data + this->count - 1); }
    inline Type* last_ref() { return this->data + this->count - 1; }
    inline bool is_empty() const { return this->count == 0; }
    inline bool is_initialized() const { return this->data != null && this->alloc != null; }
    inline void clear() { this->count = 0; }
    inline sz byte_size_used() const { return this->count * sizeof(Type); }
    inline sz byte_size_allocated() const { return this->capacity * sizeof(Type); }
};

template<typename Type>
DArray<Type>::DArray()
    : data{null}, alloc{null}
{
}

template<typename Type>
void DArray<Type>::init(Allocator* alloc, sz init_capacity)
{
    ASSERT_GREATER_EQ_ZERO(init_capacity);
    init_capacity = max(DARRAY_DEFAULT_CAPACITY, init_capacity);
    this->data = (Type*)allocator_allocate(alloc, init_capacity * sizeof(Type));
    this->count = 0;
    this->capacity = init_capacity;
    this->alloc = alloc;
}

template<typename Type>
void DArray<Type>::init_slice(Allocator* alloc, Slice<Type> values, sz additional_capacity)
{
    ASSERT_NON_EMPTY_VAL(values);
    sz init_cap = max(DARRAY_DEFAULT_CAPACITY, values.count + additional_capacity);
    this->data = (Type*)allocator_allocate(alloc, init_cap * sizeof(Type));
    this->count = 0;
    this->capacity = init_cap;
    this->alloc = alloc;
    if (values.count)
    {
        this->push(values);
    }
}

template<typename Type>
void DArray<Type>::push(Type value)
{
    ASSERT_INITIALIZED(this);
    this->reserve(1);
    *this->end() = value;
    this->count++;
}

template<typename Type>
void DArray<Type>::push(Slice<Type> input)
{
    ASSERT_INITIALIZED(this);
    this->reserve(input.count);
    Type* curr = this->end();
    Type* inp_curr = input.ptr;
    mem_copy(curr, inp_curr, input.byte_size());
    this->count += input.count;
}

template<typename Type>
Type DArray<Type>::pop()
{
    ASSERT_GREATER_ZERO(this->count);
    Type val = this->last();
    this->count--;
    return val;
}

template<typename Type>
Type DArray<Type>::remove_unordered_at(sz idx)
{
    ASSERT_IN_BOUNDS(idx >= 0 && idx < this->count);

    if (idx == this->count - 1)
    {
        return this->pop();
    }

    this->swap(idx, this->count - 1);
    Type val = this->last();
    this->count--;
    return val;
}

template<typename Type>
inline Type DArray<Type>::at(sz idx) const
{
    ASSERT_IN_BOUNDS(idx >= 0 && idx < this->count);
    return this->data[idx];
}

template<typename Type>
inline Type* DArray<Type>::at_ref(sz idx)
{
    ASSERT_IN_BOUNDS(idx >= 0 && idx < this->count);
    return this->data + idx;
}

template<typename Type>
inline void DArray<Type>::set(Type val, sz idx)
{
    ASSERT_IN_BOUNDS(idx >= 0 && idx < this->count);
    Type* place = this->data + idx;
    *place = val;
}

template<typename Type>
inline void DArray<Type>::swap(sz idx1, sz idx2)
{
    ASSERT_IN_BOUNDS(idx1 >= 0 && idx1 < this->count);
    ASSERT_IN_BOUNDS(idx2 >= 0 && idx2 < this->count);
    rg::swap(this->at_ref(idx1), this->at_ref(idx2));
}

template<typename Type>
inline Type DArray<Type>::operator[](sz idx) const
{
    return this->at(idx);
}

template<typename Type>
void DArray<Type>::reserve(sz needed)
{
    sz remain = this->capacity - this->count;
    if (remain >= needed) return;

    sz old_capacity = this->capacity;
    sz min_required = old_capacity + needed;
    sz new_capacity = old_capacity;

    if (new_capacity == 0) new_capacity = DARRAY_DEFAULT_CAPACITY;
    while (new_capacity < min_required)
    {
        new_capacity *= 2;
    }
    
    this->data = (Type*)allocator_reallocate(this->alloc, this->data, sizeof(Type) * new_capacity);
    this->capacity = new_capacity;
}

template<typename Type>
Slice<Type> DArray<Type>::slice(sz start, sz end)
{
    if (end == -1) end = this->count;
    sz dist = end - start;
    ASSERT_GREATER_ZERO(dist);
    Type* data = this->data;
    return Slice{ data + start, dist };
}

template<typename Type>
void DArray<Type>::trim_end_n(sz trim_count)
{
    ASSERT_MSG(trim_count < this->count, "Shouldn't exceed inner count");
    this->count -= trim_count;
}

template<typename Type>
void DArray<Type>::trim_value_set_end(Slice<Type> trim_vals)
{
    ASSERT_MSG(trim_vals.count, "Should be more than 0 values");

    Type curr;
    sz trim_count = this->count - 1;
    bool was_trim = false;

    while (trim_count >= 0)
    {
        curr = this->at(trim_count);
        for (Type val : trim_vals)
        {
            if (val == curr)
            {
                was_trim = true;
                break;
            }
        }
        // Return case.
        if (!was_trim)
        {
            this->count -= trim_count;
            return;
        }
        --trim_count;
    }
}

template<typename Type>
bool DArray<Type>::trim_sequence_end(Slice<Type> trim_vals)
{
    if (!this->ends_with(trim_vals)) return false;
    this->count -= trim_vals.count;
}

template<typename Type>
void DArray<Type>::trim_from_end_to_first_occur(Type search, bool inclusive)
{
    sz idx = this->index_of(search);
    if (idx == INDEX_INVALID || idx == 0) return;
    if (inclusive) idx++;
    this->count = idx;
}

template<typename Type>
void DArray<Type>::trim_from_end_to_last_occur(Type search, bool inclusive)
{
    sz idx = this->last_index_of(search);
    if (idx == INDEX_INVALID || idx == 0) return;
    if (inclusive) idx++;
    this->count = idx;
}


template<typename Type>
sz DArray<Type>::index_of(Type search) const
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized");

    for (sz i = 0; i < this->count; ++i)
    {
        if (this->at(i) == search) return i;
    }
    return INDEX_INVALID;
}

template<typename Type>
sz DArray<Type>::index_of(Slice<Type> slice) const
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized");
    if (slice.count == 1) return this->index_of(slice[0]);

    Type* curr;
    Type* inp_curr = slice.at_ref(1);
    Type match_start = slice[0];
    Type* end = this->end();
    Type* inp_end = slice->end();
    sz i = 0;

    for (; i < this->count && (this->count - i) >= slice.count; ++i)
    {
        if (this->at(i) == match_start)
        {
            curr = this->at_ref(i + 1);
            while (inp_curr != inp_end && curr != end && *curr == *inp_curr)
            {
                ++curr;
                ++inp_curr;
            }
            // Test for success.
            if (inp_curr == inp_end) return i;
            inp_curr = slice.at_ref(1);
        }
    }
    return INDEX_INVALID;
}

template<typename Type>
sz DArray<Type>::last_index_of(Type search) const
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized");

    sz i = this->count - 1;
    for (; i >= 0; --i)
    {
        if (this->at(i) == search) return i;
    }
    return INDEX_INVALID;
}

template<typename Type>
sz DArray<Type>::last_index_of(Slice<Type> seq) const
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized");
    if (seq.count == 1) return this->last_index_of(seq[0]);

    Type match_start = seq.last();
    Type* curr;
    Type* inp_curr = seq.last_ref() - 1;
    Type* begin = this->begin() - 1;
    Type* inp_begin = seq->begin() - 1;
    sz i = this->count - 1;
    sz j;

    for (; i >= 0 && (i+1) >= seq.count; --i)
    {
        if (this->at(i) == match_start)
        {
            j = i;
            curr = this->at_ref(j - 1);
            while (inp_curr != inp_begin && curr != begin && *curr == *inp_curr)
            {
                --curr;
                --inp_curr;
                --j;
            }
            // Test for success.
            if (inp_curr == inp_begin) return j + 1;
            inp_curr = seq.last_ref() - 1;
        }
    }
    return INDEX_INVALID;
}

template<typename Type>
bool DArray<Type>::has(Type search) const
{
    return this->index_of(search) != INDEX_INVALID;
}

template<typename Type>
bool DArray<Type>::has(Slice<Type> slice) const
{
    return this->index_of(slice) != INDEX_INVALID;
}

template<typename Type>
void DArray<Type>::replace(Type find, Type replace)
{
    for (Type* val = this->begin(); val != this->end(); ++val)
    {
        if (*val == find) *val = replace;
    }
}

template<typename Type>
bool DArray<Type>::starts_with(Slice<Type> input) const
{
    if (input.count > this->count) return false;
    return mem_compare(this->data, input.ptr, input.byte_size());
}

template<typename Type>
bool DArray<Type>::ends_with(Slice<Type> input) const
{
    if (input.count > this->count) return false;
    Type* start = this->at_ref(this->count - input.count);
    return mem_compare(start, input.ptr, input.byte_size());
}

template<typename Type>
void DArray<Type>::move_ownership(DArray<Type>* other)
{
    this = other;
    other = null;
}

template<typename Type>
void DArray<Type>::foreach(void(*fn)(Type))
{
    for (Type* curr = this->begin(); curr != this->end(); ++curr)
    {
        fn(*curr);
    }
}

template<typename Type>
void DArray<Type>::foreach_ref(void(*fn)(Type*))
{
    for (Type* curr = this->begin(); curr != this->end(); ++curr)
    {
        fn(curr);
    }
}

template<typename Type>
SplitIterator<Type> DArray<Type>::get_split_iter(Type splitter)
{
    return { this->slice(), splitter };
}

template<typename Type>
void DArray<Type>::foreach_split(Type splitter, void(*fn)(Slice<Type>))
{
    SplitIterator<Type> iter = this->get_split_iter(splitter);
    Slice<Type> seq;

    for (;;)
    {
        seq = iter.next();
        if (seq.is_empty()) return;
        fn(seq);
    }
}

template<typename Type>
void DArray<Type>::destroy()
{
    if (this->data)
    {
        this->count = 0;
        this->capacity = 0;
        allocator_free(this->alloc, this->data);
        this->data = null;
    }
}

} // rg

#endif // _RG_DARRAY_HPP_
