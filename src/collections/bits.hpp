#ifndef _RG_BITS_HPP_
#define _RG_BITS_HPP_

#include <math.h>
#include "collections/farray.hpp"
#include "core/atomic.hpp"
#include "core/basic.hpp"

namespace rg
{

// Popcount builtins.

template<typename Type>
intern sz software_popcount(Type active_storage)
{
    sz count = 0;
    while (active_storage)
    {
        active_storage &= (active_storage - 1); // Clears the lowest set bit
        count++;
    }
    return count;
}

template<typename Type>
sz bit_popcount(Type storage);
template<>
sz bit_popcount(u64 storage);
template<>
sz bit_popcount(u32 storage);
template<>
sz bit_popcount(u16 storage);
template<>
sz bit_popcount(u8 storage);

// BitInt.
// Use integer type as bit representation.
// Maximum bit count here is 64, if need more - use BitSet.

template<typename Type = u64>
struct BitInt
{
    static_assert(sizeof(Type) <= 8, "Mustn't exceed 64 bit for this type");
private:
    Type storage;
public:
    BitInt(): storage{0} {}
    Type get_mask(Type mask);
    void set_mask(Type value, Type mask);
    void set(sz idx);
    void unset(sz idx);
    bool is_set(sz idx);

    void set_all()
    {
        this->storage |= ~Type(0);
    }
    void unset_all()
    {
        this->storage &= Type(0);
    }
    bool is_any_set() { return this->set_bit_count() > 0; }
    bool is_nothing_set() { return this->set_bit_count() == 0; }
    sz set_bit_count() { return rg::bit_popcount(this->storage); }
};

template<typename Type>
Type BitInt<Type>::get_mask(Type mask)
{
    s32 ctz = __builtin_ctz(mask);
    Type res = storage & mask;
    return res >> ctz;
}

template<typename Type>
void BitInt<Type>::set_mask(Type value, Type mask)
{
    storage &= ~mask; 
    s32 ctz = __builtin_ctz(mask);
    storage |= mask & (value << ctz);
}

template<typename Type>
void BitInt<Type>::set(sz idx)
{
    ASSERT_IN_BOUNDS(idx >= 0 && idx < sizeof(Type) * 8);
    storage |= Type(1 << (Type)idx);
}

template<typename Type>
void BitInt<Type>::unset(sz idx)
{
    ASSERT_IN_BOUNDS(idx >= 0 && idx < sizeof(Type) * 8);
    storage &= ~(Type(1 << (Type)idx));
}

template<typename Type>
bool BitInt<Type>::is_set(sz idx)
{
    ASSERT_IN_BOUNDS(idx >= 0 && idx < sizeof(Type) * 8);
    return storage & Type(1 << (Type)idx);
}

// Atomic bit int.

template<typename Type = u64>
struct AtomicBitInt
{
    static_assert(sizeof(Type) <= 8, "Mustn't exceed 64 bit for this type");
private:
    Atomic<Type> storage;
public:
    AtomicBitInt(): storage{0} {}
    Type get_mask(Type mask);
    void set_mask(Type value, Type mask);
    void set(sz idx);
    void unset(sz idx);
    bool is_set(sz idx);

    void set_all()
    {
        storage.bit_or(~Type(0));
    }
    void unset_all()
    {
        storage.bit_and(Type(0));
    }
    bool is_any_set() { return this->set_bit_count() > 0; }
    bool is_nothing_set() { return this->set_bit_count() == 0; }
    sz set_bit_count() { return rg::bit_popcount(this->storage.load()); }
};

template<typename Type>
Type AtomicBitInt<Type>::get_mask(Type mask)
{
    s32 ctz = __builtin_ctz(mask);
    Type res = storage.bit_and(mask);
    return res >> ctz;
}

template<typename Type>
void AtomicBitInt<Type>::set_mask(Type value, Type mask)
{
    Type old_val = storage.load();
    Type new_val; 
    do
    {
        new_val = old_val;
        new_val &= ~mask;
        s32 ctz = __builtin_ctz(mask);
        new_val |= mask & (value << ctz);
    } while (storage.compare_exchange_weak(&old_val, new_val));
}

template<typename Type>
void AtomicBitInt<Type>::set(sz idx)
{
    ASSERT_IN_BOUNDS(idx >= 0 && idx < sizeof(Type) * 8);
    Type old_val = storage.load();
    Type new_val; 
    do
    {
        new_val = old_val;
        new_val |= Type(1 << (Type)idx);
    } while (storage.compare_exchange_weak(&old_val, new_val));
}

template<typename Type>
void AtomicBitInt<Type>::unset(sz idx)
{
    ASSERT_IN_BOUNDS(idx >= 0 && idx < sizeof(Type) * 8);
    Type old_val = storage.load();
    Type new_val; 
    do
    {
        new_val = old_val;
        new_val &= ~(Type(1 << (Type)idx));
    } while (storage.compare_exchange_weak(&old_val, new_val));
}

template<typename Type>
bool AtomicBitInt<Type>::is_set(sz idx)
{
    ASSERT_IN_BOUNDS(idx >= 0 && idx < sizeof(Type) * 8);
    return storage.load() & Type(1 << (Type)idx);
}

// Bitset.

template<typename BucketType, sz BIT_COUNT>
struct BitSet
{
private:
    static constexpr sz BUCKET_BIT_SIZE = bitsizeof(BucketType);
    static constexpr sz BUCKET_COUNT = rg::max((sz)::ceil(BIT_COUNT / (f32)BUCKET_BIT_SIZE), (sz)1);
    Array<BucketType, BUCKET_COUNT> bit_buckets;
public:
    BitSet(): bit_buckets{0} {}
    void set(sz idx)
    {
        auto [bucket_idx, bit_idx] = this->calc_indices(idx);
        this->bit_buckets[bucket_idx] |= (BucketType)1 << bit_idx;
    }
    void unset(sz idx)
    {
        auto [bucket_idx, bit_idx] = this->calc_indices(idx);
        this->bit_buckets[bucket_idx] &= ~((BucketType)1 << bit_idx);
    }
    void set_all()
    {
        rg::mem_set(this->bit_buckets, sizeof(BucketType) * BUCKET_COUNT, ~(u8)0);
    }
    void unset_all()
    {
        rg::mem_zero(this->bit_buckets, sizeof(BucketType) * BUCKET_COUNT);
    }
    bool is_set(sz idx)
    {
        auto [bucket_idx, bit_idx] = this->calc_indices(idx);
        BucketType bucket = this->bit_buckets[bucket_idx];
        return bucket & ((BucketType)1 << bit_idx);
    }
    sz set_bit_count()
    {
        sz res = 0;
        for (BucketType b : this->bit_buckets)
        {
            res += rg::bit_popcount(b);    
        }
        return res;
    }
    bool is_any_set() { return this->set_bit_count() > 0; }
    bool is_nothing_set() { return this->set_bit_count() == 0; }
    Pair<sz, BucketType> calc_indices(sz idx)
    {
        return { (idx / BUCKET_BIT_SIZE), BucketType(idx % BUCKET_BIT_SIZE) };
    }
};

// Growable bitset.

template<typename BucketType>
struct DBitSet
{
    static constexpr sz BUCKET_BIT_SIZE = bitsizeof(BucketType);
    static constexpr sz DEFAULT_BIT_CAPACITY = 64;
    static constexpr sz bits_to_elements(sz bit_capacity)
    {
        return rg::max((sz)::ceil(bit_capacity / (f32)BUCKET_BIT_SIZE), (sz)1);
    }
    static constexpr sz bits_to_elements_floor(sz bit_capacity)
    {
        return rg::max((sz)::floor(bit_capacity / (f32)BUCKET_BIT_SIZE), (sz)1);
    }
private:
    Slice<BucketType> bit_buckets;
    Allocator* alloc;
public:
    DBitSet(): bit_buckets{}, alloc{null} {}
    void init(Allocator* alloc, sz bit_capacity = DEFAULT_BIT_CAPACITY);
    void set(sz input_bit_idx);
    void unset(sz input_bit_idx);
    bool is_set(sz input_bit_idx);
    void resize(sz need_bits);
    void set_all();
    void unset_all();
    sz set_bit_count();
    sz count_trailing_zeroes(bool should_set_bit = false, sz offset_bits = 0);
    sz count_trailing_ones(sz offset_bits = 0);
    sz count_leading_zeroes(bool should_set_bit = false);
    sz count_leading_ones();
    void destroy();

    sz current_bit_capacity() { this->bit_buckets.byte_size() * 8; }
    bool is_any_set() { return this->set_bit_count() > 0; }
    bool is_nothing_set() { return this->set_bit_count() == 0; }
    Pair<sz, BucketType> calc_indices(sz idx)
    {
        return { (idx / BUCKET_BIT_SIZE), BucketType(idx % BUCKET_BIT_SIZE) };
    }
};

template<typename BucketType>
void DBitSet<BucketType>::init(Allocator* alloc, sz bit_capacity)
{
    this->alloc = alloc;
    this->bit_buckets = Slice<BucketType>::make(alloc, bits_to_elements(bit_capacity));
}

template<typename BucketType>
void DBitSet<BucketType>::destroy()
{
    if (this->bit_buckets)
    {
        allocator_free(this->alloc, this->bit_buckets);
        this->bit_buckets = {};
    }
}

template<typename BucketType>
void DBitSet<BucketType>::set(sz input_bit_idx)
{
    if (input_bit_idx > this->current_bit_capacity()) this->resize(input_bit_idx - this->current_bit_capacity());
    auto [bucket_idx, bit_idx] = this->calc_indices(input_bit_idx);
    this->bit_buckets[bucket_idx] |= (BucketType)1 << bit_idx;
}

template<typename BucketType>
void DBitSet<BucketType>::resize(sz need_bits)
{
    ASSERT_GREATER_ZERO(need_bits);
    sz need_capacity = bits_to_elements(need_bits) + this->bit_buckets.count;
    sz new_capacity = this->bit_buckets.count;
    sz old_capacity = new_capacity;

    if (!new_capacity) new_capacity = bits_to_elements(DEFAULT_BIT_CAPACITY);
    // Linear growth intentionally, we don't need exponential in a bitset.
    while (new_capacity < need_capacity) new_capacity++;

    this->bit_buckets = { (BucketType*)allocator_reallocate(this->alloc, this->bit_buckets.ptr, new_capacity * sizeof(BucketType)), new_capacity };
    // Zero new memory.
    mem_zero(this->bit_buckets.ptr + old_capacity, sizeof(BucketType) * (new_capacity - old_capacity));
}

template<typename BucketType>
void DBitSet<BucketType>::unset(sz input_bit_idx)
{
    ASSERT_MSG(input_bit_idx <= this->current_bit_capacity(), "Mustn't exceed current bit capacity");

    auto [bucket_idx, bit_idx] = this->calc_indices(input_bit_idx);
    this->bit_buckets[bucket_idx] &= ~((BucketType)1 << bit_idx);
}

template<typename BucketType>
bool DBitSet<BucketType>::is_set(sz input_bit_idx)
{
    auto [bucket_idx, bit_idx] = this->calc_indices(input_bit_idx);
    BucketType bucket = this->bit_buckets[bucket_idx];
    return bucket & ((BucketType)1 << bit_idx);
}

template<typename BucketType>
void DBitSet<BucketType>::set_all()
{
    rg::mem_set(this->bit_buckets.ptr, sizeof(BucketType) * this->bit_buckets.count, ~(u8)0);
}

template<typename BucketType>
void DBitSet<BucketType>::unset_all()
{
    rg::mem_zero(this->bit_buckets, sizeof(BucketType) * this->bit_buckets.count);
}

template<typename BucketType>
sz DBitSet<BucketType>::set_bit_count()
{
    sz res = 0;
    for (BucketType b : this->bit_buckets)
    {
        res += rg::bit_popcount(b);    
    }
    return res;
}

template<typename BucketType>
sz DBitSet<BucketType>::count_trailing_zeroes(bool should_set_bit, sz offset_bits)
{
    sz i = 0;
    if (offset_bits != 0) i = bits_to_elements_floor(offset_bits);

    for (; i < this->bit_buckets.count; ++i)
    {
        BucketType bucket = this->bit_buckets[i];
        if (bucket == 0) return bitsizeof(BucketType) * i;
        sz ctz = __builtin_ctz(bucket);
        if (ctz != 0)
        {
            if (should_set_bit) bucket |= (BucketType)1 << ctz;
            return bitsizeof(BucketType) * i + ctz;
        }
    }
    return 0;
}

template<typename BucketType>
sz DBitSet<BucketType>::count_trailing_ones(sz offset_bits)
{
    sz i = 0;
    if (offset_bits != 0) i = bits_to_elements_floor(offset_bits);

    for (; i < this->bit_buckets.count; ++i)
    {
        // Invert the bucket.
        BucketType bucket_inv = ~(this->bit_buckets[i]);
        if (bucket_inv == 0) return bitsizeof(BucketType) * i;
        sz ctz = __builtin_ctz(bucket_inv);
        if (ctz != 0)
        {
            return bitsizeof(BucketType) * i + ctz;
        }
    }
    return 0;
}

template<typename BucketType>
sz DBitSet<BucketType>::count_leading_zeroes(bool should_set_bit)
{
    sz i = this->bit_buckets.count - 1;
    sz j;
    for (; i >= 0; --i)
    {
        j = this->bit_buckets.count - 1 - i;
        BucketType bucket = this->bit_buckets[i];
        if (bucket == 0) return bitsizeof(BucketType) * j;
        sz clz = __builtin_clz(bucket);
        if (clz != 0) return bitsizeof(BucketType) * j + clz;
    }
    return 0;
}

template<typename BucketType>
sz DBitSet<BucketType>::count_leading_ones()
{
    sz i = this->bit_buckets.count - 1;
    sz j;
    for (; i >= 0; --i)
    {
        j = this->bit_buckets.count - 1 - i;
        BucketType bucket_inv = ~(this->bit_buckets[i]);
        if (bucket_inv == 0) return bitsizeof(BucketType) * j;
        sz clz = __builtin_clz(bucket_inv);
        if (clz != 0) return bitsizeof(BucketType) * j + clz;
    }
    return 0;
}

// Common masks.

enum BitMask8 : u8
{
    BIT_MASK_8_LOW_1   = 0x01,
    BIT_MASK_8_HIGH_1  = 0x10,
    BIT_MASK_8_EMPTY   = 0x00,
    BIT_MASK_8_FULL    = 0xFF,
};

enum BitMask16 : u16
{
    BIT_MASK_16_LOW_1   = 0x0001,
    BIT_MASK_16_HIGH_1  = 0x1000,
    BIT_MASK_16_LOW_8   = 0x00FF,
    BIT_MASK_16_HIGH_8  = 0xFF00,
    BIT_MASK_16_EMPTY   = 0x0000,
    BIT_MASK_16_FULL    = 0xFFFF,
};

enum BitMask32 : u32
{
    BIT_MASK_32_LOW_1   = 0x00000001,
    BIT_MASK_32_HIGH_1  = 0x10000000,
    BIT_MASK_32_LOW_8   = 0x000000FF,
    BIT_MASK_32_HIGH_8  = 0xFF000000,
    BIT_MASK_32_LOW_16  = 0x0000FFFF,
    BIT_MASK_32_HIGH_16 = 0xFFFF0000,
    BIT_MASK_32_LOW_24  = 0x00FFFFFF,
    BIT_MASK_32_HIGH_24 = 0xFFFFFF00,
    BIT_MASK_32_EMPTY   = 0xFFFFFFFF,
    BIT_MASK_32_FULL    = 0xFFFFFFFF,
};

enum BitMask64 : u64
{
    BIT_MASK_64_LOW_1   = 0x0000000000000001,
    BIT_MASK_64_HIGH_1  = 0x1000000000000000,
    BIT_MASK_64_LOW_8   = 0x00000000000000FF,
    BIT_MASK_64_HIGH_8  = 0xFF00000000000000,
    BIT_MASK_64_LOW_16  = 0x000000000000FFFF,
    BIT_MASK_64_HIGH_16 = 0xFFFF000000000000,
    BIT_MASK_64_LOW_32  = 0x00000000FFFFFFFF,
    BIT_MASK_64_HIGH_32 = 0xFFFFFFFF00000000,
    BIT_MASK_64_LOW_56  = 0x00FFFFFFFFFFFFFF,
    BIT_MASK_64_HIGH_56 = 0xFFFFFFFFFFFFFF00,
    BIT_MASK_64_EMPTY   = 0x0000000000000000,
    BIT_MASK_64_FULL    = 0xFFFFFFFFFFFFFFFF,
};

} // rg

#endif // _RG_BITS_HPP_
