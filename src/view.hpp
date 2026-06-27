#ifndef _RG_VIEW_HPP_
#define _RG_VIEW_HPP_

#include "basic.hpp"

// View.

namespace rg
{

template <typename Type>
struct View
{
    Type* ptr;
    sz count;

    inline void trim_start(sz count);
    inline void trim_end(sz count);
    inline Type get(sz idx);
    inline Type* get_ref(sz idx);
    inline void set(Type val, sz idx);
    inline Type operator[](sz idx);
    bool operator==(View other);

    inline Type* data() { return ptr; }
    inline Type* begin() { return ptr; }
    inline Type* end() { return ptr + count; }
    inline Type first() { return *ptr; }
    inline Type* first_ref() { return ptr; }
    inline Type last() { return *(ptr + count - 1); }
    inline Type* last_ref() { return ptr + count - 1; }
};

template <typename Type>
View<Type> view_create(Type* ptr, sz count)
{
    return View{ ptr, count };
}

template <typename Type>
inline void View<Type>::trim_start(sz trim_count)
{
    ASSERT_MSG(trim_count < this->count, "Shouldn't exceed inner count");
    this->ptr += trim_count;
    this->count -= trim_count;
}

template <typename Type>
inline void View<Type>::trim_end(sz trim_count)
{
    ASSERT_MSG(trim_count < this->count, "Shouldn't exceed inner count");
    this->count -= trim_count;
}

template <typename Type>
inline Type View<Type>::get(sz idx)
{
    return this->get(idx);
}

template <typename Type>
inline Type* View<Type>::get_ref(sz idx)
{
    return &this->get(idx);
}

template <typename Type>
inline void View<Type>::set(Type val, sz idx)
{
    Type* target = this->get_ref(idx);
    *target = val;
}

template <typename Type>
inline Type View<Type>::operator[](sz idx)
{
    return this->get(idx);
}

template <typename Type>
bool View<Type>::operator==(View other)
{
    if (this->count != other.count) return false;
    return rg::mem_compare(this->ptr, other.ptr, other.count);
}

} // rg

#endif // _RG_VIEW_HPP_
