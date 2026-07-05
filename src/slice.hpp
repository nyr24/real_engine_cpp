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
    // Trims 'count' characters from start.
    void trim_start_n(sz count);
    // Trims 'count' characters from end.
    void trim_end_n(sz count);
    // Trims any value from the set. (input isn't considered sequential)
    void trim_value_set_start(Slice<Type> value_set);
    // Trims any value from the set. (input isn't considered sequential)
    void trim_value_set_end(Slice<Type> value_set);
    // Trims sequentially values from the start of input. (input is considered sequential)
    void trim_sequence_start(Slice<Type> seq);
    // Trims sequentially values from the end of input. (input is considered sequential)
    void trim_sequence_end(Slice<Type> seq);
    void trim_from_start_to_first_occur(Type search, bool inclusive = false);
    void trim_from_start_to_last_occur(Type search, bool inclusive = false);
    void trim_from_end_to_first_occur(Type search, bool inclusive = false);
    void trim_from_end_to_last_occur(Type search, bool inclusive = false);
    // Trim is inplace, slice is copy.
    Slice slice(sz start = 0, sz offset = INDEX_INVALID) const;
    Slice slice_idx(sz start_idx = 0, sz end_idx = INDEX_INVALID) const;
    bool starts_with(Slice<Type> input) const;
    bool ends_with(Slice<Type> input) const;
    sz index_of(Type val) const;
    sz index_of(Slice<Type> slice) const;
    sz last_index_of(Type val) const;
    sz last_index_of(Slice<Type> slice) const;
    bool has(Type val) const;
    bool has(Slice<Type> val) const;
    void replace(Type find, Type replace);
    u64 hash() const;

    inline Type at(sz idx) const;
    inline Type* at_ref(sz idx);
    inline void set(Type val, sz idx);
    inline Type operator[](sz idx) const;
    inline Type* data() { return this->ptr; }
    inline Type* begin() { return this->ptr; }
    inline const Type* begin() const { return this->ptr; }
    inline Type* end() { return this->ptr + this->count; }
    inline const Type* end() const { return this->ptr + this->count; }
    inline Type first() const { return *this->ptr; }
    inline Type* first_ref() { return this->ptr; }
    inline Type last() const { return *(this->ptr + this->count - 1); }
    inline Type* last_ref() { return this->ptr + this->count - 1; }
    inline bool is_initialized() const { return this->ptr && this->count; }
    inline bool is_empty() const { return this->ptr == null && this->count == 0; }
    inline sz byte_size() const { return sizeof(Type) * this->count; }
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
void Slice<Type>::trim_start_n(sz trim_count)
{
    ASSERT_MSG(trim_count < this->count, "Shouldn't exceed inner count");
    this->ptr += trim_count;
    this->count -= trim_count;
}

template<typename Type>
void Slice<Type>::trim_end_n(sz trim_count)
{
    ASSERT_MSG(trim_count < this->count, "Shouldn't exceed inner count");
    this->count -= trim_count;
}

template<typename Type>
void Slice<Type>::trim_value_set_start(Slice<Type> trim_vals)
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
void Slice<Type>::trim_value_set_end(Slice<Type> trim_vals)
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
void Slice<Type>::trim_from_start_to_first_occur(Type search, bool inclusive)
{
    sz idx = this->index_of(search);
    if (idx == INDEX_INVALID || idx == 0) return;
    if (!inclusive) idx++;
    this->ptr += idx;
}

template<typename Type>
void Slice<Type>::trim_from_start_to_last_occur(Type search, bool inclusive)
{
    sz idx = this->last_index_of(search);
    if (idx == INDEX_INVALID || idx == 0) return;
    if (!inclusive) idx++;
    this->ptr += idx;
}

template<typename Type>
void Slice<Type>::trim_from_end_to_first_occur(Type search, bool inclusive)
{
    sz idx = this->index_of(search);
    if (idx == INDEX_INVALID || idx == 0) return;
    if (inclusive) idx++;
    this->count = idx;
}

template<typename Type>
void Slice<Type>::trim_from_end_to_last_occur(Type search, bool inclusive)
{
    sz idx = this->last_index_of(search);
    if (idx == INDEX_INVALID || idx == 0) return;
    if (inclusive) idx++;
    this->count = idx;
}

template<typename Type>
sz Slice<Type>::index_of(Type search) const
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized");

    for (sz i = 0; i < this->count; ++i)
    {
        if (this->at(i) == search) return i;
    }
    return INDEX_INVALID;
}

template<typename Type>
sz Slice<Type>::index_of(Slice<Type> slice) const
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized");
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

template<typename Type>
sz Slice<Type>::last_index_of(Type search) const
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized");

    sz i = this->count - 1;
    for (; i >= 0; --i)
    {
        if (this->at(i) == search) return i;
    }
    return INDEX_INVALID;
}

template<typename Type>
sz Slice<Type>::last_index_of(Slice<Type> seq) const
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized");
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

template<typename Type>
bool Slice<Type>::has(Type search) const
{
    return this->index_of(search) != INDEX_INVALID;
}

template<typename Type>
bool Slice<Type>::has(Slice<Type> slice) const
{
    return this->index_of(slice) != INDEX_INVALID;
}

template<typename Type>
Slice<Type> Slice<Type>::slice(sz start, sz offset) const
{
    if (offset == INDEX_INVALID) offset = this->count;
    return Slice{ this->ptr + start, offset };
}

template<typename Type>
Slice<Type> Slice<Type>::slice_idx(sz start_idx, sz end_idx) const
{
    if (end_idx == INDEX_INVALID) end_idx = this->count - 1;
    ASSERT_MSG(end_idx > start_idx, "End must be greater than start");

    end_idx = (end_idx - start_idx) + 1;
    return Slice{ this->ptr + start_idx, end_idx };
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
bool Slice<Type>::starts_with(Slice<Type> input) const
{
    if (input.count > this->count) return false;
    return mem_compare(this->ptr, input.ptr, input.byte_size());
}

template<typename Type>
bool Slice<Type>::ends_with(Slice<Type> input) const
{
    if (input.count > this->count) return false;
    Type* start = this->at_ref(this->count - input.count);
    return mem_compare(start, input.ptr, input.byte_size());
}

template<typename Type>
inline Type Slice<Type>::at(sz idx) const
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
inline Type Slice<Type>::operator[](sz idx) const
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
u64 Slice<Type>::hash() const
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
    bool starts_with(StrView input);
    bool starts_with(CString input);
    // Removes const qualifier from pointer, be careful.
    inline Slice<u8> to_byte_slice_unsafe() { return { (u8*)this->ptr, this->count }; }
    // Removes const qualifier from pointer, be careful.
    inline Slice<char> to_char_slice_unsafe() { return { (char*)this->ptr, this->count }; }
};

} // rg

#endif // _RG_SLICE_HPP_
