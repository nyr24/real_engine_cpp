#ifndef _RG_SLICE_HPP_
#define _RG_SLICE_HPP_

#include "core/basic.hpp"

// Slice.

namespace rg
{

template<typename Type>
struct Slice
{
    static constexpr sz DEFAULT_MAKE_CAPACITY = 8;
    
    Type* ptr;
    sz count;

    Slice() = default;
    Slice(Type* ptr, sz count);

    static Slice make(Allocator* alloc, sz capacity = DEFAULT_MAKE_CAPACITY);
    void init(Type* ptr, sz count);
    // Trims 'count' characters from start.
    void trim_start_n(sz count);
    // Trims 'count' characters from end.
    void trim_end_n(sz count);
    Slice slice(sz start = 0, sz offset = INDEX_INVALID) const;
    Slice slice_idx(sz start_idx = 0, sz end_idx = INDEX_INVALID) const;
    Maybe<sz> index_of(Type val) const;
    Maybe<sz> last_index_of(Type val) const;
    bool has(Type val) const;
    u64 hash() const;
    Slice<u8> to_byte_slice();

    Type at(sz idx) const;
    Type* at_ref(sz idx);
    const Type* at_ref(sz idx) const;
    void set(Type val, sz idx);
    const Type& operator[](sz idx) const { return this->ptr[idx]; };
    Type& operator[](sz idx) { return this->ptr[idx]; };
    Type* data() { return this->ptr; }
    Type* begin() { return this->ptr; }
    const Type* begin() const { return this->ptr; }
    Type* end() { return this->ptr + this->count; }
    const Type* end() const { return this->ptr + this->count; }
    Type first() const { return *this->ptr; }
    Type* first_ref() { return this->ptr; }
    const Type* first_ref() const { return this->ptr; }
    Type last() const { return *(this->ptr + this->count - 1); }
    Type* last_ref() { return this->ptr + this->count - 1; }
    const Type* last_ref() const { return this->ptr + this->count - 1; }
    bool is_initialized() const { return this->ptr && this->count; }
    bool is_empty() const { return this->ptr == null && this->count == 0; }
    sz byte_size() const { return sizeof(Type) * this->count; }
    operator bool() const { return this->ptr && this->count; }
};

template<typename Type>
struct View : Slice<const Type>
{
    using Slice<Type>::Slice;
};

// For printf formatting with length (%.*s).
#define FMT_SLICE(slice) (s32)slice.count, slice.ptr
#define FMT_SLICE_PTR(slice) (s32)slice->count, slice->ptr

// Common code.

template<typename Type>
void common_trim_start_n(const Type** RESTRICT start, sz* item_count, sz trim_count);
template<typename Type>
void common_trim_end_n(const Type** RESTRICT start, sz* item_count, sz trim_count);
template<typename Type>
Maybe<sz> common_index_of(const Type* start, sz count, Type search);
template<typename Type>
Maybe<sz> common_last_index_of(const Type* start, sz count, Type search);
template<typename Type>
bool common_has(const Type* start, sz count, Type slice);
template<typename Type>
bool common_has(const Type* RESTRICT start, sz count, Slice<Type> slice);

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
Slice<Type> Slice<Type>::make(Allocator* alloc, sz capacity)
{
    Slice<Type> res;
    res.ptr = (Type*)allocator_allocate(alloc, capacity * sizeof(Type));
    res.count = capacity;
    return res;
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
    common_trim_end_n(&this->ptr, &this->count, trim_count);
}

template<typename Type>
Maybe<sz> Slice<Type>::index_of(Type search) const
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized");
    return common_index_of(this->ptr, this->count, search);
}

template<typename Type>
Maybe<sz> Slice<Type>::last_index_of(Type search) const
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized");
    return common_last_index_of(this->ptr, this->count, search);
}

template<typename Type>
bool Slice<Type>::has(Type search) const
{
    return common_has(this->ptr, this->count, search);
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
Type Slice<Type>::at(sz idx) const
{
    return this->ptr[idx];
}

template<typename Type>
Type* Slice<Type>::at_ref(sz idx)
{
    return this->ptr + idx;
}

template<typename Type>
const Type* Slice<Type>::at_ref(sz idx) const
{
    return this->ptr + idx;
}

template<typename Type>
void Slice<Type>::set(Type val, sz idx)
{
    Type* target = this->at_ref(idx);
    *target = val;
}

template<typename Type>
Slice<u8> Slice<Type>::to_byte_slice()
{
    return Slice<u8>{ (u8*)this->ptr, this->count * sizeof(Type) };
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
void common_trim_start_n(const Type** RESTRICT start, sz* item_count, sz trim_count)
{
    *start += trim_count;
    *item_count -= trim_count;
}

// Trims 'count' characters from start.
template<typename Type>
void common_trim_end_n(const Type** RESTRICT start, sz* item_count, sz trim_count)
{
    *item_count -= trim_count;
}

template<typename Type>
Maybe<sz> common_index_of(const Type* start, sz count, Type search)
{
    Maybe<sz> res;
    for (sz i = 0; i < count; ++i)
    {
        if (start[i] == search)
        {
            res.set_val(i);
            return res;
        }
    }
    return res;
}

template<typename Type>
Maybe<sz> common_last_index_of(const Type* start, sz count, Type search)
{
    Maybe<sz> res;
    sz i = count - 1;
    for (; i >= 0; --i)
    {
        if (start[i] == search)
        {
            res.set_val(i);
            return res;
        }
    }
    return res;
}

template<typename Type>
bool common_has(const Type* start, sz count, Type search)
{
    auto [_, has] = common_index_of(start, count, search);
    return has;
}

template<typename Type>
bool common_has(const Type* RESTRICT start, sz count, Slice<Type> slice)
{
    auto [_, has] = common_index_of(start, count, slice);
    return has;
}

} // rg

#endif // _RG_SLICE_HPP_
