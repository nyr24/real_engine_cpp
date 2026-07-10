#ifndef _RG_SLICE_HPP_
#define _RG_SLICE_HPP_

#include "core/basic.hpp"

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
    // Trims sequentially values from the start of input. (input is considered sequential)
    bool trim_sequence_start(Slice<Type> seq);
    // Trims sequentially values from the end of input. (input is considered sequential)
    bool trim_sequence_end(Slice<Type> seq);
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

    Type at(sz idx) const;
    Type* at_ref(sz idx);
    void set(Type val, sz idx);
    Type operator[](sz idx) const;
    Type* data() { return this->ptr; }
    Type* begin() { return this->ptr; }
    const Type* begin() const { return this->ptr; }
    Type* end() { return this->ptr + this->count; }
    const Type* end() const { return this->ptr + this->count; }
    Type first() const { return *this->ptr; }
    Type* first_ref() { return this->ptr; }
    Type last() const { return *(this->ptr + this->count - 1); }
    Type* last_ref() { return this->ptr + this->count - 1; }
    bool is_initialized() const { return this->ptr && this->count; }
    bool is_empty() const { return this->ptr == null && this->count == 0; }
    sz byte_size() const { return sizeof(Type) * this->count; }
};

#define FMT_SLICE(slice) (s32)slice.count, slice.ptr
#define FMT_SLICE_PTR(slice) (s32)slice->count, slice->ptr

// Predeclare common calls.

// Trims 'count' characters from start.
template<typename Type>
inline void common_trim_start_n(Type** start, sz* item_count, sz trim_count);
// Trims 'count' characters from start.
template<typename Type>
inline void common_trim_end_n(Type** start, sz* item_count, sz trim_count);
template<typename Type>
bool common_starts_with(Type* ptr, sz count, Slice<Type> input);
template<typename Type>
bool common_ends_with(Type* ptr, sz count, Slice<Type> input);
template<typename Type>
bool common_trim_sequence_start(Type** ptr, sz* count, Slice<Type> trim_seq);
template<typename Type>
bool common_trim_sequence_end(Type** ptr, sz* count, Slice<Type> trim_seq);
template<typename Type>
void common_trim_from_start_to_first_occur(Type** start, sz* count, Type search, bool inclusive);
template<typename Type>
void common_trim_from_start_to_last_occur(Type** start, sz* count, Type search, bool inclusive);
template<typename Type>
void common_trim_from_end_to_first_occur(Type** start, sz* count, Type search, bool inclusive);
template<typename Type>
void common_trim_from_end_to_last_occur(Type** start, sz* count, Type search, bool inclusive);
template<typename Type>
sz common_index_of(Type* start, sz count, Type search);
template<typename Type>
sz common_index_of(Type* start, sz count, Slice<Type> slice);
template<typename Type>
sz common_last_index_of(Type* start, sz count, Type search);
template<typename Type>
sz common_last_index_of(Type* start, sz count, Slice<Type> seq);
template<typename Type>
bool common_has(Type* start, sz count, Type search);
template<typename Type>
bool common_has(Type* start, sz count, Slice<Type> slice);

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
    common_trim_start_n(&this->ptr, &this->count, trim_count);
}

template<typename Type>
void Slice<Type>::trim_end_n(sz trim_count)
{
    ASSERT_MSG(trim_count < this->count, "Shouldn't exceed inner count");
    common_trim_end_n(&this->ptr, this->count, trim_count);
}

template<typename Type>
bool Slice<Type>::trim_sequence_start(Slice<Type> trim_seq)
{
    return common_trim_sequence_start(&this->ptr, &this->count, trim_seq);
}

template<typename Type>
bool Slice<Type>::trim_sequence_end(Slice<Type> trim_seq)
{
    return common_trim_sequence_end(&this->ptr, &this->count, trim_seq);
}

template<typename Type>
void Slice<Type>::trim_from_start_to_first_occur(Type search, bool inclusive)
{
    return common_trim_from_start_to_first_occur(&this->ptr, &this->count, search, inclusive);
}

template<typename Type>
void Slice<Type>::trim_from_start_to_last_occur(Type search, bool inclusive)
{
    return common_trim_from_start_to_last_occur(&this->ptr, &this->count, search, inclusive);
}

template<typename Type>
void Slice<Type>::trim_from_end_to_first_occur(Type search, bool inclusive)
{
    return common_trim_from_end_to_first_occur(&this->ptr, &this->count, search, inclusive);
}

template<typename Type>
void Slice<Type>::trim_from_end_to_last_occur(Type search, bool inclusive)
{
    return common_trim_from_end_to_last_occur(&this->ptr, &this->count, search, inclusive);
}

template<typename Type>
sz Slice<Type>::index_of(Type search) const
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized");
    return common_index_of(&this->ptr, this->count, search);
}

template<typename Type>
sz Slice<Type>::index_of(Slice<Type> slice) const
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized");
    return common_index_of(&this->ptr, this->count, slice);
}

template<typename Type>
sz Slice<Type>::last_index_of(Type search) const
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized");
    return common_last_index_of(&this->ptr, this->count, search);
}

template<typename Type>
sz Slice<Type>::last_index_of(Slice<Type> slice) const
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized");
    return common_last_index_of(&this->ptr, this->count, slice);
}

template<typename Type>
bool Slice<Type>::has(Type search) const
{
    return common_has(&this->ptr, this->count, search);
}

template<typename Type>
bool Slice<Type>::has(Slice<Type> slice) const
{
    return common_has(&this->ptr, this->count, slice);
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
    return common_starts_with(&this->ptr, this->count, input);
}

template<typename Type>
bool Slice<Type>::ends_with(Slice<Type> input) const
{
    return common_ends_with(&this->ptr, this->count, input);
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

// Common array operations so different containers can share them.

// Trims 'count' characters from start.
template<typename Type>
inline void common_trim_start_n(Type** start, sz* item_count, sz trim_count)
{
    *start += trim_count;
    *item_count -= trim_count;
}

// Trims 'count' characters from start.
template<typename Type>
inline void common_trim_end_n(Type** start, sz* item_count, sz trim_count)
{
    *item_count -= trim_count;
}

template<typename Type>
bool common_starts_with(Type* ptr, sz count, Slice<Type> input)
{
    if (input.count > count) return false;
    return mem_compare((void*)ptr, (void*)input.ptr, input.byte_size());
}

template<typename Type>
bool common_ends_with(Type* ptr, sz count, Slice<Type> input)
{
    if (input.count > count) return false;
    Type* start = ptr + (count - input.count);
    return mem_compare(start, input.ptr, input.byte_size());
}

template<typename Type>
bool common_trim_sequence_start(Type** ptr, sz* count, Slice<Type> trim_seq)
{
    if (!common_starts_with(*ptr, count, trim_seq)) return false;
    *ptr += trim_seq.count;
    *count -= trim_seq.count;
}

template<typename Type>
bool common_trim_sequence_end(Type** ptr, sz* count, Slice<Type> trim_seq)
{
    if (!common_ends_with(*ptr, count, trim_seq)) return false;
    count -= trim_seq.count;
}

template<typename Type>
void common_trim_from_start_to_first_occur(Type** start, sz* count, Type search, bool inclusive)
{
    sz idx = common_index_of(*start, *count, search);
    if (idx == INDEX_INVALID || idx == 0) return;
    if (!inclusive) idx++;
    *start += idx;
    *count -= idx;
}

template<typename Type>
void common_trim_from_start_to_last_occur(Type** start, sz* count, Type search, bool inclusive)
{
    sz idx = common_last_index_of(*start, *count, search);
    if (idx == INDEX_INVALID || idx == 0) return;
    if (!inclusive) idx++;
    *start += idx;
    *count -= idx;
}

template<typename Type>
void common_trim_from_end_to_first_occur(Type** start, sz* count, Type search, bool inclusive)
{
    sz idx = common_index_of(*start, *count, search);
    if (idx == INDEX_INVALID || idx == 0) return;
    if (inclusive) idx++;
    *count = idx;
}

template<typename Type>
void common_trim_from_end_to_last_occur(Type** start, sz* count, Type search, bool inclusive)
{
    sz idx = common_index_of(*start, *count, search);
    if (idx == INDEX_INVALID || idx == 0) return;
    if (inclusive) idx++;
    *count = idx;
}

template<typename Type>
sz common_index_of(Type* start, sz count, Type search)
{
    for (sz i = 0; i < count; ++i)
    {
        if (start[i] == search) return i;
    }
    return INDEX_INVALID;
}

template<typename Type>
sz common_index_of(Type* start, sz count, Slice<Type> slice)
{
    if (slice.count == 1) return common_index_of(slice[0]);

    Type* curr;
    Type* inp_curr = slice.at_ref(1);
    Type match_start = slice[0];
    Type* end = start + count;
    Type* inp_end = slice->end();
    sz i = 0;

    for (; i < count && (count - i) >= slice.count; ++i)
    {
        if (start[i] == match_start)
        {
            curr = start + (i + 1);
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
sz common_last_index_of(Type* start, sz count, Type search)
{
    sz i = count - 1;
    for (; i >= 0; --i)
    {
        if (start[i] == search) return i;
    }
    return INDEX_INVALID;
}

template<typename Type>
sz common_last_index_of(Type* start, sz count, Slice<Type> seq)
{
    if (seq.count == 1) return common_last_index_of(seq[0]);

    Type match_start = seq.last();
    Type* curr;
    Type* inp_curr = seq.last_ref() - 1;
    Type* begin = start - 1;
    Type* inp_begin = seq->begin() - 1;
    sz i = count - 1;
    sz j;

    for (; i >= 0 && (i+1) >= seq.count; --i)
    {
        if (start[i] == match_start)
        {
            j = i;
            curr = start + (j - 1);
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
bool common_has(Type* start, sz count, Type search)
{
    return common_index_of(start, count, search) != INDEX_INVALID;
}

template<typename Type>
bool common_has(Type* start, sz count, Slice<Type> slice)
{
    return common_index_of(start, count, slice) != INDEX_INVALID;
}

} // rg

#endif // _RG_SLICE_HPP_
