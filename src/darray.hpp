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
    Slice<Type> slice(sz start = 0, sz offset = -1) const;
    Slice<Type> slice_idx(sz start = 0, sz end = -1) const;
    Slice<Type> slice_start_n(sz count);
    Slice<Type> slice_sequence_start(Slice<Type> value_set);
    Slice<Type> slice_from_start_to_first_occur(Type search, bool inclusive = false);
    Slice<Type> slice_from_start_to_last_occur(Type search, bool inclusive = false);
    // Trims 'count' items from end.
    void trim_end_n(sz count);
    // Trims items sequentially from the end. (input is considered sequential)
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
Slice<Type> DArray<Type>::slice(sz start, sz offset) const
{
    if (offset == -1) offset = this->count;
    ASSERT_GREATER_ZERO(offset);
    return Slice{ this->data + start, offset };
}

template<typename Type>
Slice<Type> DArray<Type>::slice_idx(sz start, sz end) const
{
    if (end == -1) end = this->count;
    sz dist = end - start;
    ASSERT_GREATER_ZERO(dist);
    Type* data = this->data;
    return Slice{ data + start, dist };
}

template<typename Type>
Slice<Type> DArray<Type>::slice_start_n(sz trim_count)
{
    ASSERT_MSG(trim_count < this->count, "Shouldn't exceed inner count");
    Slice<Type> slice = this->slice();
    slice.trim_start_n(trim_count);
    return slice;
}

template<typename Type>
void DArray<Type>::trim_end_n(sz trim_count)
{
    ASSERT_MSG(trim_count < this->count, "Shouldn't exceed inner count");
    common_trim_end_n(&this->data, this->count, trim_count);
}

template<typename Type>
Slice<Type> DArray<Type>::slice_sequence_start(Slice<Type> trim_seq)
{
    Slice<Type> slice = this->slice();
    slice.trim_sequence_start(trim_seq);
    return slice;
}

template<typename Type>
bool DArray<Type>::trim_sequence_end(Slice<Type> trim_seq)
{
    return common_trim_sequence_end(&this->data, &this->count, trim_seq);
}

template<typename Type>
Slice<Type> DArray<Type>::slice_from_start_to_first_occur(Type search, bool inclusive)
{
    Slice<Type> slice = this->slice();
    slice.trim_from_start_to_first_occur(search, inclusive);
    return slice;
}

template<typename Type>
Slice<Type> DArray<Type>::slice_from_start_to_last_occur(Type search, bool inclusive)
{
    Slice<Type> slice = this->slice();
    slice.slice_from_start_to_last_occur(search, inclusive);
    return slice;
}

template<typename Type>
void DArray<Type>::trim_from_end_to_first_occur(Type search, bool inclusive)
{
    return common_trim_from_end_to_first_occur(&this->data, &this->count, search, inclusive);
}

template<typename Type>
void DArray<Type>::trim_from_end_to_last_occur(Type search, bool inclusive)
{
    return common_trim_from_end_to_last_occur(&this->data, &this->count, search, inclusive);
}

template<typename Type>
sz DArray<Type>::index_of(Type search) const
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized");
    return common_index_of(&this->data, this->count, search);
}

template<typename Type>
sz DArray<Type>::index_of(Slice<Type> slice) const
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized");
    return common_index_of(&this->data, this->count, slice);
}

template<typename Type>
sz DArray<Type>::last_index_of(Type search) const
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized");
    return common_last_index_of(&this->data, this->count, search);
}

template<typename Type>
sz DArray<Type>::last_index_of(Slice<Type> slice) const
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized");
    return common_last_index_of(&this->data, this->count, slice);
}

template<typename Type>
bool DArray<Type>::has(Type search) const
{
    return common_has(&this->data, this->count, search);
}

template<typename Type>
bool DArray<Type>::has(Slice<Type> slice) const
{
    return common_has(&this->data, this->count, slice);
}

template<typename Type>
bool DArray<Type>::starts_with(Slice<Type> input) const
{
    return common_starts_with(&this->data, this->count, input);
}

template<typename Type>
bool DArray<Type>::ends_with(Slice<Type> input) const
{
    return common_ends_with(&this->data, this->count, input);
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
