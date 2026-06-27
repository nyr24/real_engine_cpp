#ifndef _RG_DYNARR_HPP_
#define _RG_DYNARR_HPP_

#include "basic.hpp"
#include "view.hpp"

// Dynarr.

namespace rg
{

internal const sz DEFAULT_INIT_CAPACITY = 16;

template <typename Type>
struct DynArray
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
    void swap_ownership(DynArray<Type>* other);
    void move_ownership(DynArray<Type>* other);
    View<Type> view(sz start = 0, sz end = -1);

    inline Type get(sz idx);
    inline Type* get_ref(sz idx);
    inline void set(Type val, sz idx);
    inline void swap(sz idx1, sz idx2);
    inline Type operator[](sz idx);

    inline sz len() { return this->count; }
    inline Type* data() { return (Type*)((u8*)this + sizeof(DynArray<Type>)); }
    inline Type* begin() { return this->data(); }
    inline Type* end() { return this->data() + this->count; }
    inline Type first() { return *this->data(); }
    inline Type* first_ref() { return this->data(); }
    inline Type last() { return *(this->data() + this->count - 1); }
    inline Type* last_ref() { return this->data() + this->count - 1; }
};

template <typename Type>
DynArray<Type>* dynarr_create(Allocator* alloc, sz init_capacity = DEFAULT_INIT_CAPACITY)
{
    DynArray<Type>* dynarr = (DynArray<Type>*)allocator_allocate(alloc, sizeof(DynArray<Type>) + init_capacity * sizeof(Type));
    dynarr->count = 0;
    dynarr->capacity = init_capacity;
    dynarr->alloc = alloc;
    return dynarr;
}

template <typename Type>
DynArray<Type>* dynarr_create_with_values(Allocator* alloc, View<Type> init_values = {})
{
    DynArray<Type>* dynarr = (DynArray<Type>*)allocator_allocate(alloc, sizeof(DynArray<Type>) + init_values.count * sizeof(Type));
    dynarr->count = 0;
    dynarr->capacity = init_values.count;
    
    if (init_values.count)
    {
        dynarr->push_many(init_values);
    }
    return dynarr;
}

template <typename Type>
void DynArray<Type>::push(Type value)
{
    dynarr_reserve(this, 1);
    Type* data = this->data();
    *(data + this->count) = value;
    this->count++;
}

template <typename Type>
void DynArray<Type>::push_many(View<Type> values)
{
    dynarr_reserve(this, values.count);
    Type* data = this->data();

    for (Type val : values)
    {
        *(data + this->count) = val;
        this->count++;
    }
}

template <typename Type>
Type DynArray<Type>::pop()
{
    ASSERT_MSG(this->count > 0, "Must be greater than 0");
    Type val = this->last();
    this->count--;
    return val;
}

template <typename Type>
Type DynArray<Type>::remove_unordered_at(sz idx)
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

template <typename Type>
inline Type DynArray<Type>::get(sz idx)
{
    assert(idx >= 0 && idx < this->count && "Must be in bounds");
    return this->data()[idx];
}

template <typename Type>
inline Type* DynArray<Type>::get_ref(sz idx)
{
    return &this->get(idx);
}

template <typename Type>
inline void DynArray<Type>::set(Type val, sz idx)
{
    ASSERT_MSG(idx >= 0 && idx < this->count, "Must be in bounds");
    Type* place = this->data() + idx;
    *place = val;
}

template <typename Type>
inline void DynArray<Type>::swap(sz idx1, sz idx2)
{
    ASSERT_MSG(idx1 >= 0 && idx1 < this->count, "Must be in bounds");
    ASSERT_MSG(idx2 >= 0 && idx2 < this->count, "Must be in bounds");
    rg::swap(this->get_ref(idx1), this->get_ref(idx2));
}

template <typename Type>
inline Type DynArray<Type>::operator[](sz idx)
{
    return this->get(idx);
}

template <typename Type>
internal void dynarr_reserve(DynArray<Type>* self, sz needed)
{
    sz remain = self->capacity - self->count;
    if (remain >= needed) return;

    sz old_capacity = self->capacity;
    sz min_required = old_capacity + needed;
    sz new_capacity = old_capacity;

    if (new_capacity == 0) new_capacity = DEFAULT_INIT_CAPACITY;
    while (new_capacity < min_required)
    {
        new_capacity *= 2;
    }
    
    self = (DynArray<Type>*)realloc(self, sizeof(*self) + sizeof(Type) * new_capacity);
    self->capacity = new_capacity;
}

template<typename Type>
View<Type> DynArray<Type>::view(sz start, sz end)
{
    if (end == -1) end = this->count;
    sz dist = end - start;
    ASSERT_MSG(dist > 0, "Should be greater than 0");
    Type* data = this->data();
    return View{ data + start, dist };
}

template<typename Type>
void DynArray<Type>::swap_ownership(DynArray<Type>* other)
{
    ASSERT_MSG(this != other, "Trying to swap ownership with itself");
    rg::swap(this, other);
}

template<typename Type>
void DynArray<Type>::move_ownership(DynArray<Type>* other)
{
    this = other;
    other = null;
}

template <typename Type>
void DynArray<Type>::destroy()
{
    allocator_free(this->alloc, this);
}

} // rg

#endif // _RG_DYNARR_HPP_
