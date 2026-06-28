#ifndef _RG_FARRAY_HPP_
#define _RG_FARRAY_HPP_

#include "basic.hpp"
#include "view.hpp"

// Farray - fixed array type.

namespace rg
{

constexpr sz FARRAY_DEFAULT_CAPACITY = 16;

template<typename Type, sz CAPACITY = FARRAY_DEFAULT_CAPACITY>
struct FArray
{
    sz count;
    Type data[CAPACITY];

    inline void init() { this->count = 0; }
    void init_with_values(View<Type> values);
    void push(Type value);
    void push_many(View<Type> values);
    Type pop();
    Type remove_unordered_at(sz idx);
    View<Type> view(sz start = 0, sz end = -1);
    void foreach(void(*fn)(Type&));

    inline Type get(sz idx);
    inline Type* get_ref(sz idx);
    inline void set(Type val, sz idx);
    inline void swap(sz idx1, sz idx2);
    inline Type operator[](sz idx);

    inline sz len() { return this->count; }
    inline Type* begin() { return this->data; }
    inline Type* end() { return this->data + this->count; }
    inline Type first() { return *this->data; }
    inline Type* first_ref() { return this->data; }
    inline Type last() { return *(this->data + this->count - 1); }
    inline Type* last_ref() { return this->data + this->count - 1; }
    inline bool is_empty() { return this->count == 0; }
    inline void clear() { this->count = 0; }
    inline sz remain() { CAPACITY - this->count; }
};

template<typename Type, sz CAPACITY>
void FArray<Type, CAPACITY>::init_with_values(View<Type> values)
{
    this->count = 0;
    this->push_many(values);
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
void FArray<Type, CAPACITY>::push_many(View<Type> values)
{
    ASSERT_MSG(this->remain() >= values.count, "Must have space for a push_many");
    Type* data = this->data;

    for (Type val : values)
    {
        *(data + this->count) = val;
        this->count++;
    }
}

template<typename Type, sz CAPACITY>
Type FArray<Type, CAPACITY>::pop()
{
    ASSERT_MSG(this->count > 0, "Must be greater than 0");
    Type val = this->last();
    this->count--;
    return val;
}

template<typename Type, sz CAPACITY>
Type FArray<Type, CAPACITY>::remove_unordered_at(sz idx)
{
    ASSERT_MSG(idx >= 0 && idx < this->count, "Must be in bounds");

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
inline Type FArray<Type, CAPACITY>::get(sz idx)
{
    assert(idx >= 0 && idx < this->count && "Must be in bounds");
    return this->data[idx];
}

template<typename Type, sz CAPACITY>
inline Type* FArray<Type, CAPACITY>::get_ref(sz idx)
{
    return &this->get(idx);
}

template<typename Type, sz CAPACITY>
inline void FArray<Type, CAPACITY>::set(Type val, sz idx)
{
    ASSERT_MSG(idx >= 0 && idx < this->count, "Must be in bounds");
    Type* place = this->data + idx;
    *place = val;
}

template<typename Type, sz CAPACITY>
inline void FArray<Type, CAPACITY>::swap(sz idx1, sz idx2)
{
    ASSERT_MSG(idx1 >= 0 && idx1 < this->count, "Must be in bounds");
    ASSERT_MSG(idx2 >= 0 && idx2 < this->count, "Must be in bounds");
    rg::swap(this->get_ref(idx1), this->get_ref(idx2));
}

template<typename Type, sz CAPACITY>
inline Type FArray<Type, CAPACITY>::operator[](sz idx)
{
    return this->get(idx);
}

template<typename Type, sz CAPACITY>
View<Type> FArray<Type, CAPACITY>::view(sz start, sz end)
{
    if (end == -1) end = this->count;
    sz dist = end - start;
    ASSERT_MSG(dist > 0, "Should be greater than 0");
    Type* data = this->data;
    return View{ data + start, dist };
}

template<typename Type, sz CAPACITY>
void FArray<Type, CAPACITY>::foreach(void (*fn) (Type&))
{
    for (Type& val : *this)
    {
        fn(val);
    }
}

} // rg

#endif // _RG_FARRAY_HPP_
