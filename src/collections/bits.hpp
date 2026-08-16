#ifndef _RG_BITS_HPP_
#define _RG_BITS_HPP_

#include "core/basic.hpp"
#include "core/atomic.hpp"
#include "collections/farray.hpp"
#include "core/math.hpp"

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
    explicit BitInt(Type value): storage{value} {}
    Type get_mask(Type mask) const;
    void set_mask(Type value, Type mask);
    void set(sz idx);
    void unset(sz idx);
    bool is_set(sz idx) const;

    void set_all()
    {
        this->storage |= ~Type(0);
    }
    void unset_all()
    {
        this->storage &= Type(0);
    }
    bool is_any_set() const { return this->set_bit_count() > 0; }
    bool is_nothing_set() const { return this->set_bit_count() == 0; }
    sz set_bit_count() const { return rg::bit_popcount(this->storage); }
};

template<typename Type>
Type BitInt<Type>::get_mask(Type mask) const
{
    s32 ctz = rg::ctz(mask);
    Type res = this->storage & mask;
    return res >> ctz;
}

template<typename Type>
void BitInt<Type>::set_mask(Type value, Type mask)
{
    this->storage &= ~mask; 
    s32 ctz = rg::ctz(mask);
    this->storage |= mask & (value << ctz);
}

template<typename Type>
void BitInt<Type>::set(sz idx)
{
    ASSERT_IN_BOUNDS(idx >= 0 && idx < sizeof(Type) * 8);
    this->storage |= Type(1 << (Type)idx);
}

template<typename Type>
void BitInt<Type>::unset(sz idx)
{
    ASSERT_IN_BOUNDS(idx >= 0 && idx < sizeof(Type) * 8);
    this->storage &= ~(Type(1 << (Type)idx));
}

template<typename Type>
bool BitInt<Type>::is_set(sz idx) const
{
    ASSERT_IN_BOUNDS(idx >= 0 && idx < sizeof(Type) * 8);
    return this->storage & Type(1 << (Type)idx);
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
    Type get_mask(Type mask) const;
    void set_mask(Type value, Type mask);
    void set(sz idx);
    void unset(sz idx);
    bool is_set(sz idx) const;

    void set_all()
    {
        storage.bit_or(~Type(0));
    }
    void unset_all()
    {
        storage.bit_and(Type(0));
    }
    bool is_any_set() const { return this->set_bit_count() > 0; }
    bool is_nothing_set() const { return this->set_bit_count() == 0; }
    sz set_bit_count() const { return rg::bit_popcount(this->storage.load()); }
};

template<typename Type>
Type AtomicBitInt<Type>::get_mask(Type mask) const
{
    s32 ctz = rg::ctz(mask);
    Type res = this->storage.bit_and(mask);
    return res >> ctz;
}

template<typename Type>
void AtomicBitInt<Type>::set_mask(Type value, Type mask)
{
    Type old_val = this->storage.load();
    Type new_val; 
    do
    {
        new_val = old_val;
        new_val &= ~mask;
        s32 ctz = rg::ctz(mask);
        new_val |= mask & (value << ctz);
    } while (this->storage.compare_exchange_weak(&old_val, new_val));
}

template<typename Type>
void AtomicBitInt<Type>::set(sz idx)
{
    ASSERT_IN_BOUNDS(idx >= 0 && idx < sizeof(Type) * 8);
    Type old_val = this->storage.load();
    Type new_val; 
    do
    {
        new_val = old_val;
        new_val |= Type(1 << (Type)idx);
    } while (this->storage.compare_exchange_weak(&old_val, new_val));
}

template<typename Type>
void AtomicBitInt<Type>::unset(sz idx)
{
    ASSERT_IN_BOUNDS(idx >= 0 && idx < sizeof(Type) * 8);
    Type old_val = this->storage.load();
    Type new_val; 
    do
    {
        new_val = old_val;
        new_val &= ~(Type(1 << (Type)idx));
    } while (this->storage.compare_exchange_weak(&old_val, new_val));
}

template<typename Type>
bool AtomicBitInt<Type>::is_set(sz idx) const
{
    ASSERT_IN_BOUNDS(idx >= 0 && idx < sizeof(Type) * 8);
    return this->storage.load() & Type(1 << (Type)idx);
}

// Bitset.

constexpr sz ERROR_NO_ZERO_BITS = -1;
constexpr sz ERROR_NO_SET_BITS = -1;

intern constexpr sz bits_to_elements(sz bit_capacity, sz bucket_bit_size)
{
    return rg::max((sz)rg::ceil(bit_capacity / (f32)bucket_bit_size), (sz)1);
}

intern constexpr sz bits_to_elements_floor(sz bit_capacity, sz bucket_bit_size)
{
    return rg::max((sz)rg::floor(bit_capacity / (f32)bucket_bit_size), (sz)1);
}

template<typename BucketType, sz BIT_COUNT>
struct BitSet
{
private:
    static constexpr sz BUCKET_BIT_SIZE = bitsizeof(BucketType);
    static constexpr sz BUCKET_COUNT = bits_to_elements(BIT_COUNT, BUCKET_BIT_SIZE);
    static constexpr sz bit_capacity() { return BUCKET_COUNT * BUCKET_BIT_SIZE; }
    Array<BucketType, BUCKET_COUNT> bit_buckets;

public:
    BitSet(): bit_buckets{0} {}
    void set(sz idx);
    void unset(sz idx);
    void set_all();
    void clear();
    bool is_set(sz idx) const;
    sz set_bit_count() const;
    sz unset_bit_count() const;

    Maybe<sz> find_first_zero_bit(bool should_set = false, sz bit_offset = 0);
    Maybe<sz> find_first_set_bit(bool should_unset = false, sz bit_offset = 0);

    // All of these calculates one-based count, not index.
    sz count_trailing_zeroes(sz bit_offset = 0);
    sz count_trailing_ones(sz bit_offset = 0);
    sz count_leading_zeroes(sz bit_offset = 0);
    sz count_leading_ones(sz bit_offset = 0);

    bool is_all_set() const { return this->set_bit_count() == BUCKET_COUNT * BUCKET_BIT_SIZE; }
    bool is_any_set() const { return this->set_bit_count() > 0; }
    bool is_nothing_set() const { return this->set_bit_count() == 0; }
    Pair<sz, BucketType> calc_indices(sz idx)
    {
        return { (idx / BUCKET_BIT_SIZE), (BucketType)(idx % BUCKET_BIT_SIZE) };
    }
};

template<typename BucketType, sz BIT_COUNT>
void BitSet<BucketType, BIT_COUNT>::set(sz idx)
{
    auto [bucket_idx, bit_idx] = this->calc_indices(idx);
    this->bit_buckets[bucket_idx] |= (BucketType)1 << bit_idx;
}

template<typename BucketType, sz BIT_COUNT>
void BitSet<BucketType, BIT_COUNT>::unset(sz idx)
{
    auto [bucket_idx, bit_idx] = this->calc_indices(idx);
    this->bit_buckets[bucket_idx] &= ~((BucketType)1 << bit_idx);
}

template<typename BucketType, sz BIT_COUNT>
void BitSet<BucketType, BIT_COUNT>::set_all()
{
    rg::mem_set(this->bit_buckets.ptr, sizeof(BucketType) * BUCKET_COUNT, ~(u8)0);
}

template<typename BucketType, sz BIT_COUNT>
void BitSet<BucketType, BIT_COUNT>::clear()
{
    rg::mem_zero(this->bit_buckets.ptr, sizeof(BucketType) * BUCKET_COUNT);
}

template<typename BucketType, sz BIT_COUNT>
bool BitSet<BucketType, BIT_COUNT>::is_set(sz idx) const
{
    auto [bucket_idx, bit_idx] = this->calc_indices(idx);
    BucketType bucket = this->bit_buckets[bucket_idx];
    return bucket & ((BucketType)1 << bit_idx);
}

template<typename BucketType, sz BIT_COUNT>
sz BitSet<BucketType, BIT_COUNT>::set_bit_count() const
{
    sz res = 0;
    for (BucketType b : this->bit_buckets)
    {
        res += rg::bit_popcount(b);    
    }
    return res;
}

template<typename BucketType, sz BIT_COUNT>
sz BitSet<BucketType, BIT_COUNT>::unset_bit_count() const
{
    return this->bit_capacity() - this->set_bit_count();
}


template<typename BucketType, sz BIT_COUNT>
Maybe<sz> BitSet<BucketType, BIT_COUNT>::find_first_zero_bit(bool should_set_bit, sz bit_offset)
{
    Maybe<sz> res;
    sz idx = this->count_trailing_ones(bit_offset);
    if (idx >= this->bit_capacity()) return res;
    res.set_val(idx);
    if (should_set_bit) this->set(idx);
    return res;
}

template<typename BucketType, sz BIT_COUNT>
Maybe<sz> BitSet<BucketType, BIT_COUNT>::find_first_set_bit(bool should_unset_bit, sz bit_offset)
{
    Maybe<sz> res;
    sz idx = this->count_trailing_zeroes(bit_offset);
    if (idx >= this->bit_capacity()) return res;
    res.set_val(idx);
    if (should_unset_bit) this->unset(idx);
    return res;
}

template<typename BucketType, sz BIT_COUNT>
sz BitSet<BucketType, BIT_COUNT>::count_trailing_zeroes(sz bit_offset)
{
    sz i = 0;
    sz res = 0;
    sz ctz;
    BucketType* bucket;

    if (bit_offset > 0) i = bit_offset / BUCKET_BIT_SIZE;

    for (; i < this->bit_buckets.count; ++i)
    {
        bucket = &this->bit_buckets[i];
        if (*bucket == (BucketType)0) goto NEXT_ITER;
        ctz = rg::ctz(*bucket);
        if (ctz == 0) return res;
        if (ctz < BUCKET_BIT_SIZE) return res + ctz;
        NEXT_ITER:
        res += BUCKET_BIT_SIZE;
    }
    return res;
}

template<typename BucketType, sz BIT_COUNT>
sz BitSet<BucketType, BIT_COUNT>::count_trailing_ones(sz bit_offset)
{
    ASSERT_MSG(bit_offset < this->bit_capacity(), "Can't exceed bit capacity of the bitset");

    sz i = 0;
    sz res = 0;
    sz ctz;
    BucketType* bucket;
    BucketType bucket_inv;

    if (bit_offset > 0) i = bit_offset / BUCKET_BIT_SIZE;

    for (; i < this->bit_buckets.count; ++i)
    {
        bucket = &this->bit_buckets[i];
        bucket_inv = ~(*bucket);
        if (bucket_inv == (BucketType)0) goto NEXT_ITER;
        ctz = rg::ctz(bucket_inv);
        if (ctz == 0) return res;
        if (ctz < BUCKET_BIT_SIZE) return res + ctz;
        NEXT_ITER:
        res += BUCKET_BIT_SIZE;
    }
    return res;
}

template<typename BucketType, sz BIT_COUNT>
sz BitSet<BucketType, BIT_COUNT>::count_leading_zeroes(sz bit_offset)
{
    ASSERT_MSG(bit_offset < this->bit_capacity(), "Can't exceed bit capacity of the bitset");

    sz i = this->bit_buckets.count - 1;
    sz res = 0;
    sz clz;
    BucketType* bucket;

    if (bit_offset > 0) i = bit_offset / BUCKET_BIT_SIZE;

    for (; i >= 0; --i)
    {
        bucket = &this->bit_buckets[i];
        if (*bucket == 0) goto NEXT_ITER;
        clz = rg::clz(*bucket);
        if (clz == 0) return res;
        if (clz < BUCKET_BIT_SIZE) return res + clz;
        NEXT_ITER:
        res += BUCKET_BIT_SIZE;
    }
    return res;
}

template<typename BucketType, sz BIT_COUNT>
sz BitSet<BucketType, BIT_COUNT>::count_leading_ones(sz bit_offset)
{
    ASSERT_MSG(bit_offset < this->bit_capacity(), "Can't exceed bit capacity of the bitset");

    sz i = this->bit_buckets.count - 1;
    sz res = 0;
    sz clz;
    BucketType* bucket;
    BucketType bucket_inv;

    if (bit_offset > 0) i = bit_offset / BUCKET_BIT_SIZE;

    for (; i >= 0; --i)
    {
        bucket = &this->bit_buckets[i];
        bucket_inv = ~(*bucket);
        if (bucket_inv == 0) goto NEXT_ITER;
        clz = rg::clz(bucket_inv);
        if (clz == 0) return res;
        if (clz < BUCKET_BIT_SIZE) return res + clz;
        NEXT_ITER:
        res += BUCKET_BIT_SIZE;
    }
    return res;
}

// Growable bitset.
// Should use power-of-two capacity, for efficiency.

template<typename BucketType>
struct DBitSet
{
    static constexpr sz BUCKET_BIT_SIZE = bitsizeof(BucketType);
    static constexpr sz DEFAULT_BIT_CAPACITY = 64;
private:
    Slice<BucketType> bit_buckets;
    Allocator* alloc;
public:
    DBitSet(): bit_buckets{}, alloc{null} {}
    void init(Allocator* alloc, sz bit_capacity = DEFAULT_BIT_CAPACITY, bool set_all_bits = false);
    void set(sz input_bit_idx);
    void unset(sz input_bit_idx);
    bool is_set(sz input_bit_idx) const;
    void resize(sz need_bits);
    void set_all();
    void clear();
    sz set_bit_count() const;
    sz unset_bit_count() const;

    Maybe<sz> find_first_zero_bit(bool should_set_bit = false, sz bit_offset = 0);
    Maybe<sz> find_first_set_bit(bool should_unset_bit = false, sz bit_offset = 0);

    // All of these calculates 1-based count, not index.
    // If not found 0 is returned.
    sz count_trailing_zeroes(sz bit_offset = 0);
    sz count_trailing_ones(sz bit_offset = 0);
    sz count_leading_zeroes(sz bit_offset = 0);
    sz count_leading_ones(sz bit_offset = 0);
    void destroy();

    sz bit_capacity() const { return this->bit_buckets.byte_size() * 8; }
    bool is_all_set() const { return this->set_bit_count() == this->bit_capacity(); }
    bool is_any_set() const { return this->set_bit_count() > 0; }
    bool is_nothing_set() const { return this->set_bit_count() == 0; }
    Pair<sz, sz> calc_indices(sz idx) const
    {
        Pair<sz, sz> res;
        res.first = idx / BUCKET_BIT_SIZE;
        res.second = (BucketType)(idx % BUCKET_BIT_SIZE);
        return res;
    }
};

template<typename BucketType>
void DBitSet<BucketType>::init(Allocator* alloc, sz bit_capacity, bool set_all_bits)
{
    this->alloc = alloc;
    this->bit_buckets = Slice<BucketType>::make(alloc, bits_to_elements(bit_capacity, BUCKET_BIT_SIZE));
    if (set_all_bits) this->set_all();
    else this->clear();
}

template<typename BucketType>
void DBitSet<BucketType>::destroy()
{
    if (this->bit_buckets)
    {
        allocator_free(this->alloc, this->bit_buckets.ptr);
        this->bit_buckets = {};
    }
}

template<typename BucketType>
void DBitSet<BucketType>::set(sz input_bit_idx)
{
    if (input_bit_idx > this->bit_capacity()) this->resize(input_bit_idx - this->bit_capacity());
    auto [bucket_idx, bit_idx] = this->calc_indices(input_bit_idx);
    this->bit_buckets[bucket_idx] |= (BucketType)1 << bit_idx;
}

template<typename BucketType>
void DBitSet<BucketType>::resize(sz need_bits)
{
    ASSERT_GREATER_ZERO(need_bits);
    sz need_capacity = bits_to_elements(need_bits, BUCKET_BIT_SIZE) + this->bit_buckets.count;
    sz new_capacity = this->bit_buckets.count;
    sz old_capacity = new_capacity;

    if (!new_capacity) new_capacity = bits_to_elements(DEFAULT_BIT_CAPACITY, BUCKET_BIT_SIZE);
    // Linear growth intentionally, we don't need exponential in a bitset.
    while (new_capacity < need_capacity) new_capacity++;

    this->bit_buckets = { (BucketType*)allocator_reallocate(this->alloc, this->bit_buckets.ptr, new_capacity * sizeof(BucketType)), new_capacity };
    // Zero new memory.
    rg::mem_zero(this->bit_buckets.ptr + old_capacity, sizeof(BucketType) * (new_capacity - old_capacity));
}

template<typename BucketType>
void DBitSet<BucketType>::unset(sz input_bit_idx)
{
    ASSERT_MSG(input_bit_idx <= this->bit_capacity(), "Mustn't exceed current bit capacity");

    auto [bucket_idx, bit_idx] = this->calc_indices(input_bit_idx);
    this->bit_buckets[bucket_idx] &= ~((BucketType)1 << bit_idx);
}

template<typename BucketType>
bool DBitSet<BucketType>::is_set(sz input_bit_idx) const
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
void DBitSet<BucketType>::clear()
{
    rg::mem_zero(this->bit_buckets.ptr, sizeof(BucketType) * this->bit_buckets.count);
}

template<typename BucketType>
sz DBitSet<BucketType>::set_bit_count() const
{
    sz res = 0;
    for (BucketType b : this->bit_buckets)
    {
        res += rg::bit_popcount(b);    
    }
    return res;
}

template<typename BucketType>
sz DBitSet<BucketType>::unset_bit_count() const
{
    return this->bit_capacity() - this->set_bit_count();
}

template<typename BucketType>
Maybe<sz> DBitSet<BucketType>::find_first_zero_bit(bool should_set_bit, sz bit_offset)
{
    Maybe<sz> res;
    sz idx = this->count_trailing_ones(bit_offset);
    if (idx >= this->bit_capacity()) return res;
    res.set_val(idx);
    if (should_set_bit) this->set(idx);
    return res;
}

template<typename BucketType>
Maybe<sz> DBitSet<BucketType>::find_first_set_bit(bool should_unset_bit, sz bit_offset)
{
    Maybe<sz> res;
    sz idx = this->count_trailing_zeroes(bit_offset);
    if (idx >= this->bit_capacity()) return res;
    res.set_val(idx);
    if (should_unset_bit) this->unset(idx);
    return res;
}

template<typename BucketType>
sz DBitSet<BucketType>::count_trailing_zeroes(sz bit_offset)
{
    ASSERT_MSG(bit_offset < this->bit_capacity(), "Can't exceed bit capacity of the bitset");
    
    sz i = 0;
    sz res = 0;
    sz ctz;
    BucketType* bucket;

    if (bit_offset > 0) i = bit_offset / BUCKET_BIT_SIZE;

    for (; i < this->bit_buckets.count; ++i)
    {
        bucket = &this->bit_buckets[i];
        if (*bucket == (BucketType)0) goto NEXT_ITER;
        ctz = rg::ctz(*bucket);
        if (ctz == 0) return res;
        if (ctz < BUCKET_BIT_SIZE) return res + ctz;
        NEXT_ITER:
        res += BUCKET_BIT_SIZE;
    }
    return res;
}

template<typename BucketType>
sz DBitSet<BucketType>::count_trailing_ones(sz bit_offset)
{
    ASSERT_MSG(bit_offset < this->bit_capacity(), "Can't exceed bit capacity of the bitset");

    sz i = 0;
    sz res = 0;
    sz ctz;
    BucketType* bucket;
    BucketType bucket_inv;

    if (bit_offset > 0) i = bit_offset / BUCKET_BIT_SIZE;

    for (; i < this->bit_buckets.count; ++i)
    {
        bucket = &this->bit_buckets[i];
        bucket_inv = ~(*bucket);
        if (bucket_inv == (BucketType)0) goto NEXT_ITER;
        ctz = rg::ctz(bucket_inv);
        if (ctz == 0) return res;
        if (ctz < BUCKET_BIT_SIZE) return res + ctz;
        NEXT_ITER:
        res += BUCKET_BIT_SIZE;
    }
    return res;
}

template<typename BucketType>
sz DBitSet<BucketType>::count_leading_zeroes(sz bit_offset)
{
    ASSERT_MSG(bit_offset < this->bit_capacity(), "Can't exceed bit capacity of the bitset");

    sz i = this->bit_buckets.count - 1;
    sz res = 0;
    sz clz;
    BucketType* bucket;

    if (bit_offset > 0) i -= bit_offset / BUCKET_BIT_SIZE;

    for (; i >= 0; --i)
    {
        bucket = &this->bit_buckets[i];
        if (*bucket == 0) goto NEXT_ITER;
        clz = rg::clz(*bucket);
        if (clz == 0) return res;
        if (clz < BUCKET_BIT_SIZE) return res + clz;
        NEXT_ITER:
        res += BUCKET_BIT_SIZE;
    }
    return res;
}

template<typename BucketType>
sz DBitSet<BucketType>::count_leading_ones(sz bit_offset)
{
    ASSERT_MSG(bit_offset < this->bit_capacity(), "Can't exceed bit capacity of the bitset");

    sz i = this->bit_buckets.count - 1;
    sz res = 0;
    sz clz;
    BucketType* bucket;
    BucketType bucket_inv;

    if (bit_offset > 0) i -= bit_offset / BUCKET_BIT_SIZE;

    for (; i >= 0; --i)
    {
        bucket = &this->bit_buckets[i];
        bucket_inv = ~(*bucket);
        if (bucket_inv == 0) goto NEXT_ITER;
        clz = rg::clz(bucket_inv);
        if (clz == 0) return res;
        if (clz < BUCKET_BIT_SIZE) return res + clz;
        NEXT_ITER:
        res += BUCKET_BIT_SIZE;
    }
    return res;
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
