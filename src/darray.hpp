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
    sz count;
    sz capacity;
    Allocator* alloc;
    Type* data;

    void init(Allocator* alloc, sz init_capacity = DARRAY_DEFAULT_CAPACITY);
    void init_with_values(Allocator* alloc, Slice<Type> values, sz additional_capacity = 0);
    void push(Type value);
    void push_many(Slice<Type> values);
    Type pop();
    Type remove_unordered_at(sz idx);
    void reserve(sz needed);
    void destroy();
    void move_ownership(DArray<Type>* other);
    Slice<Type> slice(sz start = 0, sz end = -1);
    void foreach(void(*fn)(Type));
    void foreach_ref(void(*fn)(Type*));
    SplitIterator<Type> get_split_iter(Type splitter);
    void foreach_split(Type splitter, void(*fn)(Slice<Type>));

    inline Type at(sz idx);
    inline Type* at_ref(sz idx);
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
    inline bool is_initialized() { return this->alloc != null; }
    inline void clear() { this->count = 0; }
    inline sz byte_size_used() { return this->count * sizeof(Type); }
    inline sz byte_size_allocated() { return this->capacity * sizeof(Type); }
};

template<typename Type>
void DArray<Type>::init(Allocator* alloc, sz init_capacity)
{
    ASSERT_MSG(init_capacity >= 0, "Must be greater than 0");
    init_capacity = max(DARRAY_DEFAULT_CAPACITY, init_capacity);
    this->data = (Type*)allocator_allocate(alloc, init_capacity * sizeof(Type));
    this->count = 0;
    this->capacity = init_capacity;
    this->alloc = alloc;
}

template<typename Type>
void DArray<Type>::init_with_values(Allocator* alloc, Slice<Type> values, sz additional_capacity)
{
    ASSERT_MSG(values.is_valid(), "Values must be valid");
    sz init_cap = max(DARRAY_DEFAULT_CAPACITY, values.count + additional_capacity);
    this->data = (Type*)allocator_allocate(alloc, init_cap * sizeof(Type));
    this->count = 0;
    this->capacity = init_cap;
    this->alloc = alloc;
    if (values.count)
    {
        this->push_many(values);
    }
}

template<typename Type>
void DArray<Type>::push(Type value)
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized first");
    this->reserve(1);
    *this->end() = value;
    this->count++;
}

template<typename Type>
void DArray<Type>::push_many(Slice<Type> values)
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized first");
    this->reserve(values.count);
    Type* data = this->data;

    for (Type val : values)
    {
        *(data + this->count) = val;
        this->count++;
    }
}

template<typename Type>
Type DArray<Type>::pop()
{
    ASSERT_MSG(this->count > 0, "Must be greater than 0");
    Type val = this->last();
    this->count--;
    return val;
}

template<typename Type>
Type DArray<Type>::remove_unordered_at(sz idx)
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

template<typename Type>
inline Type DArray<Type>::at(sz idx)
{
    assert(idx >= 0 && idx < this->count && "Must be in bounds");
    return this->data[idx];
}

template<typename Type>
inline Type* DArray<Type>::at_ref(sz idx)
{
    assert(idx >= 0 && idx < this->count && "Must be in bounds");
    return this->data + idx;
}

template<typename Type>
inline void DArray<Type>::set(Type val, sz idx)
{
    ASSERT_MSG(idx >= 0 && idx < this->count, "Must be in bounds");
    Type* place = this->data + idx;
    *place = val;
}

template<typename Type>
inline void DArray<Type>::swap(sz idx1, sz idx2)
{
    ASSERT_MSG(idx1 >= 0 && idx1 < this->count, "Must be in bounds");
    ASSERT_MSG(idx2 >= 0 && idx2 < this->count, "Must be in bounds");
    rg::swap(this->at_ref(idx1), this->at_ref(idx2));
}

template<typename Type>
inline Type DArray<Type>::operator[](sz idx)
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
    ASSERT_MSG(dist > 0, "Should be greater than 0");
    Type* data = this->data;
    return Slice{ data + start, dist };
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
