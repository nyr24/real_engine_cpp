#ifndef _RG_DARRAY_HPP_
#define _RG_DARRAY_HPP_

#include "basic.hpp"
#include "view.hpp"

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

    void push(Type value);
    void push_many(View<Type> values);
    Type pop();
    Type remove_unordered_at(sz idx);
    void reserve(sz needed);
    void destroy();
    void swap_ownership(DArray<Type>* other);
    void move_ownership(DArray<Type>* other);
    View<Type> view(sz start = 0, sz end = -1);
    void foreach(void(*fn)(Type&));

    inline Type get(sz idx);
    inline Type* get_ref(sz idx);
    inline void set(Type val, sz idx);
    inline void swap(sz idx1, sz idx2);
    inline Type operator[](sz idx);

    inline sz len() { return this->count; }
    inline Type* data() { return (Type*)((u8*)this + sizeof(DArray<Type>)); }
    inline Type* begin() { return this->data(); }
    inline Type* end() { return this->data() + this->count; }
    inline Type first() { return *this->data(); }
    inline Type* first_ref() { return this->data(); }
    inline Type last() { return *(this->data() + this->count - 1); }
    inline Type* last_ref() { return this->data() + this->count - 1; }
    inline bool is_empty() { return this->count == 0; }
    inline bool is_initialized() { return this->alloc != null; }
    inline void clear() { this->count = 0; }
};

template<typename Type>
DArray<Type>* darray_create(Allocator* alloc, sz init_capacity = DARRAY_DEFAULT_CAPACITY)
{
    DArray<Type>* darray = (DArray<Type>*)allocator_allocate(alloc, sizeof(DArray<Type>) + init_capacity * sizeof(Type));
    darray->count = 0;
    darray->capacity = init_capacity;
    darray->alloc = alloc;
    return darray;
}

template<typename Type>
DArray<Type>* darray_create_with_values(Allocator* alloc, View<Type> init_values = {})
{
    DArray<Type>* darray = (DArray<Type>*)allocator_allocate(alloc, sizeof(DArray<Type>) + init_values.count * sizeof(Type));
    darray->count = 0;
    darray->capacity = init_values.count;
    darray->alloc = alloc;
    
    if (init_values.count)
    {
        darray->push_many(init_values);
    }
    return darray;
}

template<typename Type>
void DArray<Type>::push(Type value)
{
    darray_reserve(this, 1);
    Type* data = this->data();
    *(data + this->count) = value;
    this->count++;
}

template<typename Type>
void DArray<Type>::push_many(View<Type> values)
{
    darray_reserve(this, values.count);
    Type* data = this->data();

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
inline Type DArray<Type>::get(sz idx)
{
    assert(idx >= 0 && idx < this->count && "Must be in bounds");
    return this->data()[idx];
}

template<typename Type>
inline Type* DArray<Type>::get_ref(sz idx)
{
    return &this->get(idx);
}

template<typename Type>
inline void DArray<Type>::set(Type val, sz idx)
{
    ASSERT_MSG(idx >= 0 && idx < this->count, "Must be in bounds");
    Type* place = this->data() + idx;
    *place = val;
}

template<typename Type>
inline void DArray<Type>::swap(sz idx1, sz idx2)
{
    ASSERT_MSG(idx1 >= 0 && idx1 < this->count, "Must be in bounds");
    ASSERT_MSG(idx2 >= 0 && idx2 < this->count, "Must be in bounds");
    rg::swap(this->get_ref(idx1), this->get_ref(idx2));
}

template<typename Type>
inline Type DArray<Type>::operator[](sz idx)
{
    return this->get(idx);
}


template<typename Type>
void DArray<Type>::reserve(sz needed)
{
    darray_reserve(this, needed);
}

template<typename Type>
internal void darray_reserve(DArray<Type>* self, sz needed)
{
    sz remain = self->capacity - self->count;
    if (remain >= needed) return;

    sz old_capacity = self->capacity;
    sz min_required = old_capacity + needed;
    sz new_capacity = old_capacity;

    if (new_capacity == 0) new_capacity = DARRAY_DEFAULT_CAPACITY;
    while (new_capacity < min_required)
    {
        new_capacity *= 2;
    }
    
    self = (DArray<Type>*)realloc(self, sizeof(*self) + sizeof(Type) * new_capacity);
    self->capacity = new_capacity;
}

template<typename Type>
View<Type> DArray<Type>::view(sz start, sz end)
{
    if (end == -1) end = this->count;
    sz dist = end - start;
    ASSERT_MSG(dist > 0, "Should be greater than 0");
    Type* data = this->data();
    return View{ data + start, dist };
}

template<typename Type>
void DArray<Type>::swap_ownership(DArray<Type>* other)
{
    ASSERT_MSG(this != other, "Trying to swap ownership with itself");
    rg::swap(this, other);
}

template<typename Type>
void DArray<Type>::move_ownership(DArray<Type>* other)
{
    this = other;
    other = null;
}

template<typename Type>
void DArray<Type>::foreach(void (*fn) (Type&))
{
    for (Type& val : *this)
    {
        fn(val);
    }
}

template<typename Type>
void DArray<Type>::destroy()
{
    allocator_free(this->alloc, this);
}

} // rg

#endif // _RG_DARRAY_HPP_
