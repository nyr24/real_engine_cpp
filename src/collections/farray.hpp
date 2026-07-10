#ifndef _RG_FARRAY_HPP_
#define _RG_FARRAY_HPP_

#include <initializer_list>
#include "core/basic.hpp"
#include "collections/slice.hpp"
#include "collections/split_iterator.hpp"

// Farray - fixed array type.

namespace rg
{

constexpr sz FARRAY_DEFAULT_CAPACITY = 16;

template<typename Type, sz CAPACITY = FARRAY_DEFAULT_CAPACITY>
struct FArray
{
    Type data[CAPACITY];
    sz count;

    constexpr FArray(): count{0} {}
    constexpr FArray(std::initializer_list<Type> init_list);
    FArray(const FArray& rhs);
    FArray(FArray&& rhs);
    FArray& operator=(const FArray& rhs);
    FArray& operator=(FArray&& rhs);

    void init_slice(Slice<Type> values);
    void push(Type value);
    void push(Slice<Type> values);
    Type pop();
    Type remove_unordered_at(sz idx);
    Slice<Type> slice(sz start = 0, sz offset = -1) const;
    Slice<Type> slice_idx(sz start = 0, sz end = -1) const;
    Slice<Type> slice_start_n(sz count);
    Slice<Type> slice_sequence_start(Slice<Type> value_set);
    Slice<Type> slice_from_start_to_first_occur(Type search, bool inclusive = false);
    Slice<Type> slice_from_start_to_last_occur(Type search, bool inclusive = false);
    // Trims 'count' characters from end.
    void trim_end_n(sz count);
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

    Type at(sz idx) const;
    Type* at_ref(sz idx);
    void set(Type val, sz idx);
    void swap(sz idx1, sz idx2);
    Type operator[](sz idx) const;
    sz len() const { return this->count; }
    constexpr sz capacity() const { return CAPACITY; }
    Type* begin() { return this->data; }
    Type* end() { return this->data + this->count; }
    const Type* begin() const { return this->data; }
    const Type* end() const { return this->data + this->count; }
    Type first() const { return *this->data; }
    Type* first_ref() { return this->data; }
    Type last() const { return *(this->data + this->count - 1); }
    Type* last_ref() { return this->data + this->count - 1; }
    bool is_empty() const { return this->count == 0; }
    void clear() { this->count = 0; }
    sz remain() const { return CAPACITY - this->count; }
    sz byte_size_used() const { return this->count * sizeof(Type); }
    constexpr sz byte_size_allocated() const { return sizeof(Type) * CAPACITY; }
    constexpr sz byte_size_all() const { return sizeof(FArray<Type>); }
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
FArray<Type, CAPACITY>::FArray(const FArray& rhs)
    : count{rhs.count}
{
    mem_copy(this->data, rhs.data, rhs.byte_size_allocated()); 
}

template<typename Type, sz CAPACITY>
FArray<Type, CAPACITY>::FArray(FArray&& rhs)
    : count{rhs.count}
{
    mem_copy(this->data, rhs.data, rhs.byte_size_allocated()); 
}

template<typename Type, sz CAPACITY>
FArray<Type, CAPACITY>& FArray<Type, CAPACITY>::operator=(const FArray& rhs)
{
    this->count = rhs.count;
    mem_copy(this->data, rhs.data, rhs.byte_size_allocated()); 
    return *this;
}

template<typename Type, sz CAPACITY>
FArray<Type, CAPACITY>& FArray<Type, CAPACITY>::operator=(FArray&& rhs)
{
    this->count = rhs.count;
    mem_copy(this->data, rhs.data, rhs.byte_size_allocated()); 
    return *this;
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
Slice<Type> FArray<Type, CAPACITY>::slice(sz start, sz offset) const
{
    if (offset == -1) offset = this->count;
    ASSERT_GREATER_ZERO(offset);
    return Slice{ this->data + start, offset };
}

template<typename Type, sz CAPACITY>
Slice<Type> FArray<Type, CAPACITY>::slice_idx(sz start, sz end) const
{
    if (end == -1) end = this->count;
    sz dist = end - start;
    ASSERT_GREATER_ZERO(dist);
    Type* data = this->data;
    return Slice{ data + start, dist };
}

template<typename Type, sz CAPACITY>
Slice<Type> FArray<Type, CAPACITY>::slice_start_n(sz trim_count)
{
    ASSERT_MSG(trim_count < this->count, "Shouldn't exceed inner count");
    Slice<Type> slice = this->slice();
    slice.trim_start_n(trim_count);
    return slice;
}

template<typename Type, sz CAPACITY>
void FArray<Type, CAPACITY>::trim_end_n(sz trim_count)
{
    ASSERT_MSG(trim_count < this->count, "Shouldn't exceed inner count");
    common_trim_end_n(&this->data, this->count, trim_count);
}

template<typename Type, sz CAPACITY>
Slice<Type> FArray<Type, CAPACITY>::slice_sequence_start(Slice<Type> trim_seq)
{
    Slice<Type> slice = this->slice();
    slice.trim_sequence_start(trim_seq);
    return slice;
}

template<typename Type, sz CAPACITY>
bool FArray<Type, CAPACITY>::trim_sequence_end(Slice<Type> trim_seq)
{
    return common_trim_sequence_end(&this->data, &this->count, trim_seq);
}

template<typename Type, sz CAPACITY>
Slice<Type> FArray<Type, CAPACITY>::slice_from_start_to_first_occur(Type search, bool inclusive)
{
    Slice<Type> slice = this->slice();
    slice.trim_from_start_to_first_occur(search, inclusive);
    return slice;
}

template<typename Type, sz CAPACITY>
Slice<Type> FArray<Type, CAPACITY>::slice_from_start_to_last_occur(Type search, bool inclusive)
{
    Slice<Type> slice = this->slice();
    slice.slice_from_start_to_last_occur(search, inclusive);
    return slice;
}

template<typename Type, sz CAPACITY>
void FArray<Type, CAPACITY>::trim_from_end_to_first_occur(Type search, bool inclusive)
{
    return common_trim_from_end_to_first_occur(&this->data, &this->count, search, inclusive);
}

template<typename Type, sz CAPACITY>
void FArray<Type, CAPACITY>::trim_from_end_to_last_occur(Type search, bool inclusive)
{
    return common_trim_from_end_to_last_occur(&this->data, &this->count, search, inclusive);
}

template<typename Type, sz CAPACITY>
sz FArray<Type, CAPACITY>::index_of(Type search) const
{
    return common_index_of(&this->data, this->count, search);
}

template<typename Type, sz CAPACITY>
sz FArray<Type, CAPACITY>::index_of(Slice<Type> slice) const
{
    return common_index_of(&this->data, this->count, slice);
}

template<typename Type, sz CAPACITY>
sz FArray<Type, CAPACITY>::last_index_of(Type search) const
{
    return common_last_index_of(&this->data, this->count, search);
}

template<typename Type, sz CAPACITY>
sz FArray<Type, CAPACITY>::last_index_of(Slice<Type> slice) const
{
    return common_last_index_of(&this->data, this->count, slice);
}

template<typename Type, sz CAPACITY>
bool FArray<Type, CAPACITY>::has(Type search) const
{
    return common_has(&this->data, this->count, search);
}

template<typename Type, sz CAPACITY>
bool FArray<Type, CAPACITY>::has(Slice<Type> slice) const
{
    return common_has(&this->data, this->count, slice);
}

template<typename Type, sz CAPACITY>
bool FArray<Type, CAPACITY>::starts_with(Slice<Type> input) const
{
    return common_starts_with(&this->data, this->count, input);
}

template<typename Type, sz CAPACITY>
bool FArray<Type, CAPACITY>::ends_with(Slice<Type> input) const
{
    return common_ends_with(&this->data, this->count, input);
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
