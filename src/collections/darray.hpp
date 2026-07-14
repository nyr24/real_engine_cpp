#ifndef _RG_DARRAY_HPP_
#define _RG_DARRAY_HPP_

#include "core/basic.hpp"
#include "collections/slice.hpp"
#include "collections/split_iterator.hpp"

// Darray - dynamic array type.

namespace rg
{


template<typename Type>
struct DArray
{
    static constexpr sz DEFAULT_CAPACITY = 16;

    Type* data;
    Allocator* alloc;
    sz count;
    sz capacity;

    DArray();
    // Performs shallow copy (use clone() for deep).
    DArray(const DArray& rhs);
    DArray(DArray&& rhs);
    DArray& operator=(DArray&& rhs);
    // Performs shallow copy (use clone() for deep).
    DArray& operator=(const DArray& rhs);

    void init(Allocator* alloc);
    void init_capacity(Allocator* alloc, sz init_capacity = DEFAULT_CAPACITY);
    void init_slice(Allocator* alloc, Slice<Type> values, sz additional_capacity = 0);
    void push(const Type& value);
    void push(Slice<Type> values);
    void push_and_move_ownership(Type&& value);
    void push_and_move_ownership(Slice<Type> values);
    void pop(Type* out_val);
    void pop(Slice<Type> out_vals);
    void pop_and_move_ownership(Type* out_val);
    void pop_and_move_ownership(Slice<Type> out_vals);
    void remove_unordered_at(sz idx);
    void reserve(sz needed);
    void destroy();
    DArray<Type> clone();
    Slice<Type> slice(sz start = 0, sz offset = -1) const;
    Slice<Type> slice_idx(sz start = 0, sz end = -1) const;
    Slice<Type> slice_start_n(sz count);
    Slice<Type> slice_sequence_start(Slice<Type> value_set);
    Slice<Type> slice_from_start_to_first_occur(const Type& search, bool inclusive = false);
    Slice<Type> slice_from_start_to_last_occur(const Type& search, bool inclusive = false);
    // Trims 'count' items from end.
    void trim_end_n(sz count);
    // Trims items sequentially from the end. (input is considered sequential)
    bool trim_sequence_end(Slice<Type> seq);
    void trim_from_end_to_first_occur(const Type& search, bool inclusive = false);
    void trim_from_end_to_last_occur(const Type& search, bool inclusive = false);
    bool starts_with(Slice<Type> input) const;
    bool ends_with(Slice<Type> input) const;
    sz index_of(const Type& val) const;
    sz index_of(Slice<Type> slice) const;
    sz last_index_of(const Type& val) const;
    sz last_index_of(Slice<Type> slice) const;
    bool has(const Type& val) const;
    bool has(Slice<Type> val) const;
    void replace(const Type& find, const Type& replace);
    void foreach(void(*fn)(Type));
    void foreach_ref(void(*fn)(Type*));
    SplitIterator<Type> get_split_iter(const Type& splitter);
    void foreach_split(const Type& splitter, void(*fn)(Slice<Type>));

    Type at(sz idx) const;
    Type* at_ref(sz idx);
    void set(const Type& val, sz idx);
    void swap(sz idx1, sz idx2);
    Type& operator[](sz idx);
    const Type& operator[](sz idx) const;
    sz len() const { return this->count; }
    Type* begin() { return this->data; }
    Type* end() { return this->data + this->count; }
    const Type& first() const { return *this->data; }
    Type* first_ref() { return this->data; }
    const Type& last() const { return *(this->data + this->count - 1); }
    Type* last_ref() { return this->data + this->count - 1; }
    bool is_empty() const { return this->count == 0; }
    bool is_initialized() const { return this->alloc != null; }
    void clear() { this->count = 0; }
    sz byte_size_used() const { return this->count * sizeof(Type); }
    sz byte_size_allocated() const { return this->capacity * sizeof(Type); }
};

template<typename Type>
DArray<Type>::DArray()
    : data{null}, alloc{null}, count{0}, capacity{0}
{
}

template<typename Type>
DArray<Type>::DArray(DArray&& rhs)
    : data{rhs.data}, alloc{rhs.alloc}, count{rhs.count}, capacity{rhs.capacity}
{
    rhs.data = null;
    rhs.alloc = null;
    rhs.count = 0;
    rhs.capacity = 0;
}

template<typename Type>
DArray<Type>::DArray(const DArray& rhs)
    : data{rhs.data}, alloc{rhs.alloc}, count{rhs.count}, capacity{rhs.capacity}
{
}

template<typename Type>
DArray<Type>& DArray<Type>::operator=(const DArray& rhs)
{
    ASSERT_MSG(this != &rhs, "You mustn't be an idiot");
    this->data = rhs.data;
    this->alloc = rhs.alloc;
    this->count = rhs.count;
    this->capacity = rhs.capacity;
    return *this;
}

template<typename Type>
DArray<Type>& DArray<Type>::operator=(DArray&& rhs)
{
    ASSERT_MSG(this != &rhs, "You mustn't be an idiot");
    this->data = rhs.data;
    this->alloc = rhs.alloc;
    this->count = rhs.count;
    this->capacity = rhs.capacity;

    rhs.data = null;
    rhs.alloc = null;
    rhs.count = 0;
    rhs.capacity = 0;
    return *this;
}

template<typename Type>
void DArray<Type>::init(Allocator* alloc)
{
    this->alloc = alloc;
}

template<typename Type>
void DArray<Type>::init_capacity(Allocator* alloc, sz init_capacity)
{
    ASSERT_GREATER_EQ_ZERO(init_capacity);
    init_capacity = rg::max(DEFAULT_CAPACITY, init_capacity);
    this->data = (Type*)allocator_allocate(alloc, init_capacity * sizeof(Type));
    this->capacity = init_capacity;
    this->alloc = alloc;
}

template<typename Type>
void DArray<Type>::init_slice(Allocator* alloc, Slice<Type> values, sz additional_capacity)
{
    ASSERT_NON_EMPTY_VAL(values);
    sz init_cap = rg::max(DEFAULT_CAPACITY, values.count + additional_capacity);
    this->data = (Type*)allocator_allocate(alloc, init_cap * sizeof(Type));
    this->capacity = init_cap;
    this->alloc = alloc;
    if (values.count)
    {
        this->push(values);
    }
}

template<typename Type>
void DArray<Type>::push(const Type& value)
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
void DArray<Type>::push_and_move_ownership(Type&& value)
{
    ASSERT_INITIALIZED(this);
    this->reserve(1);
    *this->end() = rg::move(value);
    this->count++;
}

template<typename Type>
void DArray<Type>::pop(Type* out_val)
{
    ASSERT_GREATER_ZERO(this->count);
    if (out_val) *out_val = this->last();
    this->count--;
}

template<typename Type>
void DArray<Type>::pop(Slice<Type> out_vals)
{
    ASSERT_MSG(this->count >= out_vals.count, "Count must be greater or equal to pop count");
    if (out_vals.ptr)
    {
        sz write_idx = 0;
        while (write_idx < out_vals.count)
        {
            Type* pop_val = this->data + (this->count - 1) - write_idx;
            out_vals.ptr[write_idx] = *pop_val;
            ++write_idx;
        }
    }
    this->count -= out_vals.count;
}

template<typename Type>
void DArray<Type>::pop_and_move_ownership(Type* out_val)
{
    ASSERT_GREATER_ZERO(this->count);
    if (out_val) *out_val = rg::move(*this->last_ref());
    this->count--;
}

template<typename Type>
void DArray<Type>::pop_and_move_ownership(Slice<Type> out_vals)
{
    ASSERT_MSG(this->count >= out_vals.count, "Count must be greater or equal to pop count");
    if (out_vals.ptr)
    {
        sz write_idx = 0;
        while (write_idx < out_vals.count)
        {
            Type* pop_val = this->data + (this->count - 1) - write_idx;
            *(out_vals.ptr + write_idx) = rg::move(*pop_val);
            ++write_idx;
        }
    }
    this->count -= out_vals.count;
}

template<typename Type>
void DArray<Type>::remove_unordered_at(sz idx)
{
    ASSERT_IN_BOUNDS(idx >= 0 && idx < this->count);

    if (idx == this->count - 1)
    {
        this->count--;
        return;
    }
    this->swap(idx, this->count - 1);
    this->count--;
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
inline void DArray<Type>::set(const Type& val, sz idx)
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
inline const Type& DArray<Type>::operator[](sz idx) const
{
    return this->at(idx);
}

template<typename Type>
inline Type& DArray<Type>::operator[](sz idx)
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

    if (new_capacity == 0) new_capacity = DEFAULT_CAPACITY;
    while (new_capacity < min_required)
    {
        new_capacity *= 2;
    }
    
    if (this->data) this->data = (Type*)allocator_reallocate(this->alloc, this->data, sizeof(Type) * new_capacity);
    else this->data = (Type*)allocator_allocate(this->alloc, sizeof(Type) * new_capacity);

    this->capacity = new_capacity;
}

template<typename Type>
Slice<Type> DArray<Type>::slice(sz start, sz offset) const
{
    if (offset == -1) offset = this->count;
    ASSERT_GREATER_ZERO(offset);
    ASSERT_MSG(start + offset <= this->count, "Mustn't exceed count");
    return { this->data + start, offset };
}

template<typename Type>
Slice<Type> DArray<Type>::slice_idx(sz start, sz end) const
{
    if (end == -1) end = this->count - 1;
    sz dist = (end - start) + 1;
    ASSERT_GREATER_ZERO(dist);
    ASSERT_MSG(start + dist <= this->count, "Mustn't exceed count");
    return { this->data + start, dist };
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
    common_trim_end_n(&this->data, &this->count, trim_count);
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
Slice<Type> DArray<Type>::slice_from_start_to_first_occur(const Type& search, bool inclusive)
{
    Slice<Type> slice = this->slice();
    slice.trim_from_start_to_first_occur(search, inclusive);
    return slice;
}

template<typename Type>
Slice<Type> DArray<Type>::slice_from_start_to_last_occur(const Type& search, bool inclusive)
{
    Slice<Type> slice = this->slice();
    slice.slice_from_start_to_last_occur(search, inclusive);
    return slice;
}

template<typename Type>
void DArray<Type>::trim_from_end_to_first_occur(const Type& search, bool inclusive)
{
    return common_trim_from_end_to_first_occur(&this->data, &this->count, search, inclusive);
}

template<typename Type>
void DArray<Type>::trim_from_end_to_last_occur(const Type& search, bool inclusive)
{
    return common_trim_from_end_to_last_occur(&this->data, &this->count, search, inclusive);
}

template<typename Type>
sz DArray<Type>::index_of(const Type& search) const
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
sz DArray<Type>::last_index_of(const Type& search) const
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
bool DArray<Type>::has(const Type& search) const
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
void DArray<Type>::replace(const Type& find, const Type& replace)
{
    for (Type* val = this->begin(); val != this->end(); ++val)
    {
        if (*val == find) *val = replace;
    }
}

template<typename Type>
DArray<Type> DArray<Type>::clone()
{
    ASSERT_INITIALIZED(this);

    DArray<Type> res; 
    sz allocated = this->byte_size_allocated();
    res.data = (Type*)allocator_allocate(this->alloc, allocated);
    mem_copy(res.data, this->data, allocated);
    res.count = this->count;
    res.capacity = this->capacity;
    res.alloc = this->alloc;
    return res;
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
SplitIterator<Type> DArray<Type>::get_split_iter(const Type& splitter)
{
    return { this->slice(), splitter };
}

template<typename Type>
void DArray<Type>::foreach_split(const Type& splitter, void(*fn)(Slice<Type>))
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
