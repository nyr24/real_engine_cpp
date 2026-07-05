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
