#ifndef _RG_BITS_HPP_
#define _RG_BITS_HPP_

#include <math.h>
#include "collections/farray.hpp"
#include "core/atomic.hpp"
#include "core/basic.hpp"

namespace rg
{

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

// Popcount builtins.

template <typename Type>
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

template <typename Type>
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

template <typename Type = u64>
struct BitInt
{
    static_assert(sizeof(Type) <= 8, "Mustn't exceed 64 bit for this type");
private:
    Type storage;
public:
    BitInt(): storage{0} {}
    Type get_mask(Type mask) { return storage & mask; }
    void set_mask(Type value, Type mask)
    {
        storage &= ~mask; 
        storage |= mask & value;
    }
    void set(sz idx)
    {
        ASSERT_IN_BOUNDS(idx >= 0 && idx < sizeof(Type) * 8);
        storage |= Type(1 << (Type)idx);
    }
    void unset(sz idx)
    {
        ASSERT_IN_BOUNDS(idx >= 0 && idx < sizeof(Type) * 8);
        storage &= ~(Type(1 << (Type)idx));
    }
    void set_all()
    {
        this->storage |= ~Type(0);
    }
    void unset_all()
    {
        this->storage &= Type(0);
    }
    bool is_set(sz idx)
    {
        ASSERT_IN_BOUNDS(idx >= 0 && idx < sizeof(Type) * 8);
        return storage & Type(1 << (Type)idx);
    }
    bool is_any_set() { return this->set_bit_count() > 0; }
    bool is_nothing_set() { return this->set_bit_count() == 0; }
    sz set_bit_count() { return rg::bit_popcount(this->storage); }
};

template <typename Type = u64>
struct AtomicBitInt
{
    static_assert(sizeof(Type) <= 8, "Mustn't exceed 64 bit for this type");
private:
    Atomic<Type> storage;
public:
    AtomicBitInt(): storage{0} {}
    Type get_mask(Type mask) { return storage.bit_and(mask); }
    void set_mask(Type value, Type mask)
    {
        Type old_val = storage.load();
        Type new_val; 
        do
        {
            new_val = old_val;
            new_val &= ~mask; 
            new_val |= (mask & value);
        } while (storage.compare_exchange_weak(&old_val, new_val));
    }
    void set(sz idx)
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
    void unset(sz idx)
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
    void set_all()
    {
        storage.bit_or(~Type(0));
    }
    void unset_all()
    {
        storage.bit_and(Type(0));
    }
    bool is_set(sz idx)
    {
        ASSERT_IN_BOUNDS(idx >= 0 && idx < sizeof(Type) * 8);
        return storage.load() & Type(1 << (Type)idx);
    }
    bool is_any_set() { return this->set_bit_count() > 0; }
    bool is_nothing_set() { return this->set_bit_count() == 0; }
    sz set_bit_count() { return rg::bit_popcount(this->storage.load()); }
};

// Bitset.

template <typename BucketType, sz BIT_COUNT>
struct BitSet
{
private:
    static constexpr sz BUCKET_BIT_SIZE = sizeof(BucketType) * 8;
    static constexpr sz BUCKET_COUNT = rg::max((sz)::ceil(BIT_COUNT / (f32)BUCKET_BIT_SIZE), (sz)1);
    Array<BucketType, BUCKET_COUNT> bit_buckets;
public:
    BitSet(): bit_buckets{0} {}
    void set(sz idx)
    {
        auto [bucket_idx, bit_idx] = this->calc_indices(idx);
        this->bit_buckets[bucket_idx] |= BucketType(1) << bit_idx;
    }
    void unset(sz idx)
    {
        auto [bucket_idx, bit_idx] = this->calc_indices(idx);
        this->bit_buckets[bucket_idx] &= ~(BucketType(1) << bit_idx);
    }
    void set_all()
    {
        rg::mem_set(this->bit_buckets, sizeof(BucketType) * BUCKET_COUNT, ~u8(0));
    }
    void unset_all()
    {
        rg::mem_zero(this->bit_buckets, sizeof(BucketType) * BUCKET_COUNT);
    }
    bool is_set(sz idx)
    {
        auto [bucket_idx, bit_idx] = this->calc_indices(idx);
        BucketType bucket = this->bit_buckets[bucket_idx];
        return bucket & (BucketType(1) << bit_idx);
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

} // rg

#endif // _RG_BITS_HPP_
