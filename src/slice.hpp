#ifndef _RG_SLICE_HPP_
#define _RG_SLICE_HPP_

#include "basic.hpp"

// Slice.

namespace rg
{

template<typename Type>
struct Slice
{
    Type* ptr;
    sz count;

    Slice() = default;
    Slice(Type* ptr, sz count);

    void init(Type* ptr, sz count);
    // Trim is inplace, slice is copy.
    void trim_start(sz count);
    void trim_end(sz count);
    void trim_vals_start(Slice<Type> trim_vals);
    void trim_vals_end(Slice<Type> trim_vals);
    void trim_from(Type from);
    void trim_until(Type until, bool inclusive = false);
    Slice slice(sz start = 0, sz offset = INDEX_INVALID);
    Slice slice_idx(sz start_idx = 0, sz end_idx = INDEX_INVALID);
    Slice slice_from(Type from);
    Slice slice_until(Type until, bool inclusive = false);
    sz index_of(Type val);
    sz index_of(Slice<Type> slice);
    bool has(Type val);
    bool has(Slice<Type> val);
    void replace(Type find, Type replace);
    u64 hash();

    inline Type at(sz idx);
    inline Type* at_ref(sz idx);
    inline void set(Type val, sz idx);
    inline Type operator[](sz idx);
    inline Type* data() { return this->ptr; }
    inline Type* begin() { return this->ptr; }
    inline Type* end() { return this->ptr + this->count; }
    inline Type first() { return *this->ptr; }
    inline Type* first_ref() { return this->ptr; }
    inline Type last() { return *(this->ptr + this->count - 1); }
    inline Type* last_ref() { return this->ptr + this->count - 1; }
    inline bool is_valid() { return this->ptr && this->count; }
    inline bool is_empty() { return this->ptr == null && this->count == 0; }
};

template<typename Type>
Slice<Type>::Slice(Type* ptr, sz count)
{
    this->ptr = ptr;
    this->count = count;
}

template<typename Type>
void Slice<Type>::init(Type* ptr, sz count)
{
    this->ptr = ptr;
    this->count = count;
}

template<typename Type>
void Slice<Type>::trim_start(sz trim_count)
{
    ASSERT_MSG(trim_count < this->count, "Shouldn't exceed inner count");
    this->ptr += trim_count;
    this->count -= trim_count;
}

template<typename Type>
void Slice<Type>::trim_end(sz trim_count)
{
    ASSERT_MSG(trim_count < this->count, "Shouldn't exceed inner count");
    this->count -= trim_count;
}

template<typename Type>
void Slice<Type>::trim_vals_start(Slice<Type> trim_vals)
{
    ASSERT_MSG(trim_vals.count, "Should be more than 0 values");
    Type curr;
    sz trim_count = 0;
    bool was_trim = false;

    while (trim_count < this->count)
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
            this->ptr += trim_count;
            return;
        }
        ++trim_count;
    }
}

template<typename Type>
void Slice<Type>::trim_vals_end(Slice<Type> trim_vals)
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

template<typename Type>
void Slice<Type>::trim_from(Type from)
{
    sz idx = this->index_of(from);
    if (idx == INDEX_INVALID || idx == 0) return *this;
    this->ptr = this->at_ref(idx);
    this->count -= idx;
}

template<typename Type>
void Slice<Type>::trim_until(Type until, bool inclusive)
{
    sz idx = this->index_of(until);
    if (idx == INDEX_INVALID || idx == (this->count - 1)) return *this;
    if (inclusive) idx = min(idx + 1, this->count);
    this->count = idx;
}

template<typename Type>
sz Slice<Type>::index_of(Type search)
{
    ASSERT_MSG(this->is_valid(), "Must be initialized");
    sz i = 0;
    for (Type val : *this)
    {
        if (val == search) return i;
        ++i;
    }
    return INDEX_INVALID;
}

template<typename Type>
sz Slice<Type>::index_of(Slice<Type> slice)
{
    ASSERT_MSG(this->is_valid(), "Must be initialized");
    if (slice.count == 1) return this->index_of(slice[0]);

    Type first = slice[0];
    sz i = 0;
    Type match_start = slice[0];
    Type* end = this->end();
    Type* inp_end = slice->end();

    for (; i < this->count && (this->count - i) >= slice.count; ++i)
    {
        if (this->at(i) == match_start)
        {
            Type* curr = this->at_ref(i + 1);
            Type* inp_curr = slice.at_ref(1);
            while (curr != end && inp_curr != inp_end && *curr == *inp_curr)
            {
                ++curr;
                ++inp_curr;
            }
            // Test for success.
            if (inp_curr == inp_end) return i;
        }
    }
    return INDEX_INVALID;
}

template<typename Type>
bool Slice<Type>::has(Type search)
{
    return this->index_of(search) != INDEX_INVALID;
}

template<typename Type>
bool Slice<Type>::has(Slice<Type> slice)
{
    return this->index_of(slice) != INDEX_INVALID;
}

template<typename Type>
Slice<Type> Slice<Type>::slice(sz start, sz offset)
{
    if (offset == INDEX_INVALID) offset = this->count;
    return Slice{ this->ptr + start, offset };
}

template<typename Type>
Slice<Type> Slice<Type>::slice_idx(sz start_idx, sz end_idx)
{
    if (end_idx == INDEX_INVALID) end_idx = this->count - 1;
    ASSERT_MSG(end_idx > start_idx, "End must be greater than start");

    end_idx = (end_idx - start_idx) + 1;
    return Slice{ this->ptr + start_idx, end_idx };
}

template<typename Type>
Slice<Type> Slice<Type>::slice_from(Type from)
{
    sz idx = this->index_of(from);
    if (idx == INDEX_INVALID || idx == 0) return *this;
    sz offset = this->count - idx;
    return { this->at_ref(idx), offset };
}

template<typename Type>
Slice<Type> Slice<Type>::slice_until(Type until, bool inclusive)
{
    sz idx = this->index_of(until);
    if (idx == INDEX_INVALID || idx == (this->count - 1)) return *this;
    if (inclusive) idx = min(idx + 1, this->count);
    return { this->ptr, idx };
}

template<typename Type>
void Slice<Type>::replace(Type find, Type replace)
{
    for (Type* val = this->begin(); val != this->end(); ++val)
    {
        if (*val == find) *val = replace;
    }
}

template<typename Type>
inline Type Slice<Type>::at(sz idx)
{
    return this->ptr[idx];
}

template<typename Type>
inline Type* Slice<Type>::at_ref(sz idx)
{
    return this->ptr + idx;
}

template<typename Type>
inline void Slice<Type>::set(Type val, sz idx)
{
    Type* target = this->at_ref(idx);
    *target = val;
}

template<typename Type>
inline Type Slice<Type>::operator[](sz idx)
{
    return this->at(idx);
}

template<typename Type>
bool operator==(Slice<Type> lhs, Slice<Type> rhs)
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
u64 Slice<Type>::hash()
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

// StrView.

struct StrView : Slice<const char>
{
    StrView() = default;
    StrView(CString cstr);
    StrView(CString cstr, sz count);
    void init(CString cstr, bool preserve_null_term = true);
    void trim_until_null(bool inclusive = true);
    StrView slice_until_null(bool inclusive = true);
    bool contains_non_ascii();
};

} // rg

#endif // _RG_SLICE_HPP_
