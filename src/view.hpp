#ifndef _RG_VIEW_HPP_
#define _RG_VIEW_HPP_

#include "basic.hpp"

// View.

namespace rg
{

template<typename Type>
struct View
{
    Type* ptr;
    sz count;

    void init(Type* ptr, sz count);
    inline void trim_start(sz count);
    inline void trim_end(sz count);
    inline Type get(sz idx);
    inline Type* get_ref(sz idx);
    inline void set(Type val, sz idx);
    inline Type operator[](sz idx);
    u64 hash();

    inline Type* data() { return this->ptr; }
    inline Type* begin() { return this->ptr; }
    inline Type* end() { return this->ptr + this->count; }
    inline Type first() { return *this->ptr; }
    inline Type* first_ref() { return this->ptr; }
    inline Type last() { return *(this->ptr + this->count - 1); }
    inline Type* last_ref() { return this->ptr + this->count - 1; }
    inline bool is_valid() { return this->ptr && this->count; }
};

template<typename Type>
void View<Type>::init(Type* ptr, sz count)
{
    this->ptr = ptr;
    this->count = count;
}

template<typename Type>
View<Type> view_create(Type* ptr, sz count)
{
    return View{ ptr, count };
}

template<typename Type>
inline void View<Type>::trim_start(sz trim_count)
{
    ASSERT_MSG(trim_count < this->count, "Shouldn't exceed inner count");
    this->ptr += trim_count;
    this->count -= trim_count;
}

template<typename Type>
inline void View<Type>::trim_end(sz trim_count)
{
    ASSERT_MSG(trim_count < this->count, "Shouldn't exceed inner count");
    this->count -= trim_count;
}

template<typename Type>
inline Type View<Type>::get(sz idx)
{
    return *this->get_ref(idx);
}

template<typename Type>
inline Type* View<Type>::get_ref(sz idx)
{
    return this->ptr + idx;
}

template<typename Type>
inline void View<Type>::set(Type val, sz idx)
{
    Type* target = this->get_ref(idx);
    *target = val;
}

template<typename Type>
inline Type View<Type>::operator[](sz idx)
{
    return this->get(idx);
}

template<typename Type>
bool operator==(View<Type> lhs, View<Type> rhs)
{
    if (lhs.count != rhs.count) return false;
    Type* a = lhs.ptr;
    Type* b = rhs.ptr;
    Type* end = lhs.ptr + lhs.count;
    for (; a != end; ++a, ++b)
    {
        if (*a != *b) return false;
    }
    return true;
}

template<typename Type>
u64 View<Type>::hash()
{
    u64 hash = FNV_OFFSET_BASIS;
    sz byte_count = this->count * sizeof(Type);
    char* byte = (char*)this->ptr;
    char* end = byte + byte_count;

    for (; byte != end; ++byte)
    {
        hash ^= *byte;
        hash *= FNV_PRIME;
    }

    return hash;
}

// StrView & CStrView.

struct StrView : View<char>
{
    void init_cstr(Allocator* alloc, CString cstr);
    inline bool is_null_term() { return this->last() == '\0'; }
    StrView ensure_null_term(Allocator* alloc);
};

struct CStrView : View<const char>
{
    void init(CString cstr);
};

StrView strview_create(char* ptr, sz count);
StrView strview_create_cstr(CString cstr);
CStrView cstrview_create(CString cstr);

} // rg

#endif // _RG_VIEW_HPP_
