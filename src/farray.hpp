#ifndef _RG_FARRAY_HPP_
#define _RG_FARRAY_HPP_

#include <initializer_list>
#include "basic.hpp"
#include "slice.hpp"
#include "split_iterator.hpp"

// Farray - fixed array type.

namespace rg
{

constexpr sz FARRAY_DEFAULT_CAPACITY = 16;

template<typename Type, sz CAPACITY = FARRAY_DEFAULT_CAPACITY>
struct FArray
{
    sz count;
    Type data[CAPACITY];

    constexpr FArray() = default;
    constexpr FArray(std::initializer_list<Type> init_list);

    inline void init() { this->count = 0; }
    void init_slice(Slice<Type> values);
    void push(Type value);
    void push(Slice<Type> values);
    Type pop();
    Type remove_unordered_at(sz idx);
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
    void foreach(void(*fn)(Type)) const;
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
    inline const Type* begin() const { return this->data; }
    inline const Type* end() const { return this->data + this->count; }
    inline Type first() const { return *this->data; }
    inline Type* first_ref() { return this->data; }
    inline Type last() const { return *(this->data + this->count - 1); }
    inline Type* last_ref() { return this->data + this->count - 1; }
    inline bool is_empty() const { return this->count == 0; }
    inline void clear() { this->count = 0; }
    inline sz remain() const { return CAPACITY - this->count; }
    inline sz byte_size_used() const { return this->count * sizeof(Type); }
    constexpr inline sz byte_size_allocated() const { return sizeof(Type) * CAPACITY; }
    constexpr inline sz byte_size_all() const { return sizeof(FArray<Type>); }
};

template<typename Type, sz CAPACITY>
constexpr FArray<Type, CAPACITY>::FArray(std::initializer_list<Type> init_list)
    : count{(sz)init_list.size()}
{
    Type* start = this->data;    
    for (Type el : init_list)
    {
        *start = el;
        ++start;
    }
}

template<typename Type, sz CAPACITY>
void FArray<Type, CAPACITY>::init_slice(Slice<Type> values)
{
    this->count = 0;
    this->push(values);
}

template<typename Type, sz CAPACITY>
void FArray<Type, CAPACITY>::push(Type value)
{
    ASSERT_MSG(this->remain() >= 1, "Must have space for a push");
    Type* data = this->data;
    *(data + this->count) = value;
    this->count++;
}

template<typename Type, sz CAPACITY>
void FArray<Type, CAPACITY>::push(Slice<Type> input)
{
    ASSERT_MSG(this->remain() >= input.count, "Must have space for a push_many");
    Type* curr = this->end();
    Type* inp_curr = input.ptr;
    mem_copy(curr, inp_curr, input.byte_size());
    this->count += input.count;
}

template<typename Type, sz CAPACITY>
Type FArray<Type, CAPACITY>::pop()
{
    ASSERT_GREATER_ZERO(this->count);
    Type val = this->last();
    this->count--;
    return val;
}

template<typename Type, sz CAPACITY>
Type FArray<Type, CAPACITY>::remove_unordered_at(sz idx)
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

template<typename Type, sz CAPACITY>
inline Type FArray<Type, CAPACITY>::at(sz idx) const
{
    ASSERT_IN_BOUNDS(idx >= 0 && idx < this->count);
    return this->data[idx];
}

template<typename Type, sz CAPACITY>
inline Type* FArray<Type, CAPACITY>::at_ref(sz idx)
{
    return &this->at(idx);
}

template<typename Type, sz CAPACITY>
inline void FArray<Type, CAPACITY>::set(Type val, sz idx)
{
    ASSERT_IN_BOUNDS(idx >= 0 && idx < this->count);
    Type* place = this->data + idx;
    *place = val;
}

template<typename Type, sz CAPACITY>
inline void FArray<Type, CAPACITY>::swap(sz idx1, sz idx2)
{
    ASSERT_IN_BOUNDS(idx1 >= 0 && idx1 < this->count);
    ASSERT_IN_BOUNDS(idx2 >= 0 && idx2 < this->count);
    rg::swap(this->at_ref(idx1), this->at_ref(idx2));
}

template<typename Type, sz CAPACITY>
inline Type FArray<Type, CAPACITY>::operator[](sz idx) const
{
    return this->at(idx);
}

template<typename Type, sz CAPACITY>
Slice<Type> FArray<Type, CAPACITY>::slice(sz start, sz end)
{
    if (end == -1) end = this->count;
    sz dist = end - start;
    ASSERT_GREATER_ZERO(dist);
    Type* data = this->data;
    return Slice{ data + start, dist };
}

template<typename Type, sz CAPACITY>
void FArray<Type, CAPACITY>::trim_end_n(sz trim_count)
{
    ASSERT_MSG(trim_count < this->count, "Shouldn't exceed inner count");
    this->count -= trim_count;
}

template<typename Type, sz CAPACITY>
void FArray<Type, CAPACITY>::trim_value_set_end(Slice<Type> trim_vals)
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

template<typename Type, sz CAPACITY>
bool FArray<Type, CAPACITY>::trim_sequence_end(Slice<Type> trim_vals)
{
    if (!this->ends_with(trim_vals)) return false;
    this->count -= trim_vals.count;
}

template<typename Type, sz CAPACITY>
void FArray<Type, CAPACITY>::trim_from_end_to_first_occur(Type search, bool inclusive)
{
    sz idx = this->index_of(search);
    if (idx == INDEX_INVALID || idx == 0) return;
    if (inclusive) idx++;
    this->count = idx;
}

template<typename Type, sz CAPACITY>
void FArray<Type, CAPACITY>::trim_from_end_to_last_occur(Type search, bool inclusive)
{
    sz idx = this->last_index_of(search);
    if (idx == INDEX_INVALID || idx == 0) return;
    if (inclusive) idx++;
    this->count = idx;
}


template<typename Type, sz CAPACITY>
sz FArray<Type, CAPACITY>::index_of(Type search) const
{
    for (sz i = 0; i < this->count; ++i)
    {
        if (this->at(i) == search) return i;
    }
    return INDEX_INVALID;
}

template<typename Type, sz CAPACITY>
sz FArray<Type, CAPACITY>::index_of(Slice<Type> slice) const
{
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

template<typename Type, sz CAPACITY>
sz FArray<Type, CAPACITY>::last_index_of(Type search) const
{
    sz i = this->count - 1;
    for (; i >= 0; --i)
    {
        if (this->at(i) == search) return i;
    }
    return INDEX_INVALID;
}

template<typename Type, sz CAPACITY>
sz FArray<Type, CAPACITY>::last_index_of(Slice<Type> seq) const
{
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

template<typename Type, sz CAPACITY>
bool FArray<Type, CAPACITY>::has(Type search) const
{
    return this->index_of(search) != INDEX_INVALID;
}

template<typename Type, sz CAPACITY>
bool FArray<Type, CAPACITY>::has(Slice<Type> slice) const
{
    return this->index_of(slice) != INDEX_INVALID;
}

template<typename Type, sz CAPACITY>
void FArray<Type, CAPACITY>::replace(Type find, Type replace)
{
    for (Type* val = this->begin(); val != this->end(); ++val)
    {
        if (*val == find) *val = replace;
    }
}

template<typename Type, sz CAPACITY>
bool FArray<Type, CAPACITY>::starts_with(Slice<Type> input) const
{
    if (input.count > this->count) return false;
    return mem_compare(this->data, input.ptr, input.byte_size());
}

template<typename Type, sz CAPACITY>
bool FArray<Type, CAPACITY>::ends_with(Slice<Type> input) const
{
    if (input.count > this->count) return false;
    Type* start = this->at_ref(this->count - input.count);
    return mem_compare(start, input.ptr, input.byte_size());
}

template<typename Type, sz CAPACITY>
void FArray<Type, CAPACITY>::foreach(void (*fn) (Type)) const
{
    for (Type* curr = this->begin(); curr != this->end(); ++curr)
    {
        fn(*curr);
    }
}

template<typename Type, sz CAPACITY>
void FArray<Type, CAPACITY>::foreach_ref(void (*fn) (Type*))
{
    for (Type* curr = this->begin(); curr != this->end(); ++curr)
    {
        fn(curr);
    }
}

template<typename Type, sz CAPACITY>
SplitIterator<Type> FArray<Type, CAPACITY>::get_split_iter(Type splitter)
{
    return { this->slice(), splitter };
}

template<typename Type, sz CAPACITY>
void FArray<Type, CAPACITY>::foreach_split(Type splitter, void(*fn)(Slice<Type>))
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

// EnumArray.

template<typename Type, typename EnumType>
struct EnumArray : FArray<Type, (sz)EnumType::EnumSize>
{
    using FArray<Type, (sz)EnumType::EnumSize>::FArray;
    inline Type operator[](EnumType idx) const;
};

template<typename Type, typename EnumType>
Type EnumArray<Type, EnumType>::operator[](EnumType e_idx) const
{
    sz idx = sz(e_idx);
    ASSERT_IN_BOUNDS(idx >= 0 && idx < (sz)EnumType::EnumSize);
    return this->data[idx];
}

} // rg

#endif // _RG_FARRAY_HPP_
