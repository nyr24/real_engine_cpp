#include "core/basic.hpp"
#include "core/allocators.hpp"
#include "collections/bits.hpp"

namespace rg
{

// init.

intern void test_bitset_init(Allocator* a)
{
    DBitSet<u64> set;
    set.init(a);
    ASSERT(set.bit_capacity() == DBitSet<u64>::DEFAULT_BIT_CAPACITY);
    ASSERT(set.is_nothing_set());
    set.destroy();

    DBitSet<u64> full;
    full.init(a, 128, true);
    ASSERT(full.bit_capacity() == 128);
    ASSERT(full.is_all_set());
    full.destroy();
}

// set.

intern void test_bitset_set(Allocator* a)
{
    DBitSet<u64> set;
    set.init(a);
    set.set(0);
    set.set(63);
    ASSERT(set.is_set(0));
    ASSERT(set.is_set(63));
    ASSERT(!set.is_set(1));
    ASSERT(set.set_bit_count() == 2);

    // Setting a bit past capacity should grow the bitset.
    set.set(65);
    ASSERT(set.bit_capacity() == 128);
    ASSERT(set.is_set(0));
    ASSERT(set.is_set(65));
    ASSERT(set.set_bit_count() == 3);
    set.destroy();
}

// unset.

intern void test_bitset_unset(Allocator* a)
{
    DBitSet<u64> set;
    set.init(a);
    set.set(0);
    set.set(1);
    set.set(2);
    set.unset(1);
    ASSERT(set.is_set(0));
    ASSERT(!set.is_set(1));
    ASSERT(set.is_set(2));
    ASSERT(set.set_bit_count() == 2);
    set.destroy();
}

// is_set.

intern void test_bitset_is_set(Allocator* a)
{
    DBitSet<u64> set;
    set.init(a);
    ASSERT(!set.is_set(0));
    ASSERT(!set.is_set(63));
    set.set(10);
    ASSERT(set.is_set(10));
    ASSERT(!set.is_set(9));
    ASSERT(!set.is_set(11));
    set.unset(10);
    ASSERT(!set.is_set(10));
    set.destroy();
}

// resize.

intern void test_bitset_resize(Allocator* a)
{
    DBitSet<u64> set;
    set.init(a);
    set.set(0);

    set.resize(1);
    ASSERT(set.bit_capacity() == 128);
    ASSERT(set.is_set(0));
    ASSERT(!set.is_set(64));

    set.set(100);
    ASSERT(set.is_set(100));
    ASSERT(set.bit_capacity() == 128);

    set.resize(1);
    ASSERT(set.bit_capacity() == 192);
    ASSERT(set.is_set(0));
    ASSERT(set.is_set(100));
    set.destroy();
}

// set_all.

intern void test_bitset_set_all(Allocator* a)
{
    DBitSet<u64> set;
    set.init(a, 128);
    ASSERT(!set.is_all_set());
    set.set_all();
    ASSERT(set.is_all_set());
    ASSERT(set.set_bit_count() == 128);
    ASSERT(set.unset_bit_count() == 0);
    set.destroy();
}

// clear.

intern void test_bitset_clear(Allocator* a)
{
    DBitSet<u64> set;
    set.init(a);
    set.set(3);
    set.set(10);
    ASSERT(set.is_any_set());
    set.clear();
    ASSERT(set.is_nothing_set());
    ASSERT(set.set_bit_count() == 0);
    set.destroy();
}

// set_bit_count.

intern void test_bitset_set_bit_count(Allocator* a)
{
    DBitSet<u64> set;
    set.init(a, 128);
    ASSERT(set.set_bit_count() == 0);
    set.set(0);
    set.set(1);
    set.set(64);
    ASSERT(set.set_bit_count() == 3);
    set.set_all();
    ASSERT(set.set_bit_count() == 128);
    set.destroy();
}

// unset_bit_count.

intern void test_bitset_unset_bit_count(Allocator* a)
{
    DBitSet<u64> set;
    set.init(a, 128);
    ASSERT(set.unset_bit_count() == 128);
    set.set(0);
    set.set(100);
    ASSERT(set.unset_bit_count() == 126);
    set.set_all();
    ASSERT(set.unset_bit_count() == 0);
    set.destroy();
}

// find_first_zero_bit.

intern void test_bitset_find_first_zero_bit(Allocator* a)
{
    DBitSet<u64> set;
    set.init(a, 128);

    Maybe<sz> res = set.find_first_zero_bit();
    ASSERT(res.is_ok);
    ASSERT(res.val == 0);

    // should_set_bit.
    res = set.find_first_zero_bit(true);
    ASSERT(res.is_ok);
    ASSERT(res.val == 0);
    ASSERT(set.is_set(0));

    res = set.find_first_zero_bit(true);
    ASSERT(res.is_ok);
    ASSERT(res.val == 1);
    ASSERT(set.is_set(1));

    // On a fully set bitset there's no zero bit.
    set.set_all();
    res = set.find_first_zero_bit();
    ASSERT(!res.is_ok);
    set.destroy();
}

// find_first_set_bit.

intern void test_bitset_find_first_set_bit(Allocator* a)
{
    DBitSet<u64> set;
    set.init(a, 128);

    Maybe<sz> res = set.find_first_set_bit();
    ASSERT(!res.is_ok);

    set.set(0);
    res = set.find_first_set_bit();
    ASSERT(res.is_ok);
    ASSERT(res.val == 0);

    set.set(100);
    res = set.find_first_set_bit();
    ASSERT(res.is_ok);
    ASSERT(res.val == 0);

    set.unset(0);
    res = set.find_first_set_bit();
    ASSERT(res.is_ok);
    ASSERT(res.val == 100);

    // should_unset_bit.
    res = set.find_first_set_bit(true);
    ASSERT(res.is_ok);
    ASSERT(res.val == 100);
    ASSERT(!set.is_set(100));
    ASSERT(set.is_nothing_set());
    set.destroy();
}

// count_trailing_zeroes.

intern void test_bitset_count_trailing_zeroes(Allocator* a)
{
    DBitSet<u32> set;
    set.init(a, 64);
    ASSERT(set.count_trailing_zeroes() == 64);

    set.set(0);
    ASSERT(set.count_trailing_zeroes() == 0);

    set.clear();
    set.set(4);
    ASSERT(set.count_trailing_zeroes() == 4);

    set.clear();
    set.set(33);
    ASSERT(set.count_trailing_zeroes() == 33);
    set.destroy();
}

// count_trailing_ones.

intern void test_bitset_count_trailing_ones(Allocator* a)
{
    DBitSet<u32> set;
    set.init(a, 64);
    ASSERT(set.count_trailing_ones() == 0);

    set.set(0);
    ASSERT(set.count_trailing_ones() == 1);

    set.clear();
    set.set(0);
    set.set(1);
    set.set(2);
    ASSERT(set.count_trailing_ones() == 3);

    set.clear();
    set.set(0);
    set.set(1);
    set.set(33);
    ASSERT(set.count_trailing_ones() == 2);
    set.destroy();
}

// count_leading_zeroes.

intern void test_bitset_count_leading_zeroes(Allocator* a)
{
    DBitSet<u32> set;
    set.init(a, 64);
    ASSERT(set.count_leading_zeroes() == 64);

    set.set(0);
    ASSERT(set.count_leading_zeroes() == 63);

    set.clear();
    set.set(31);
    ASSERT(set.count_leading_zeroes() == 32);

    set.clear();
    set.set(33);
    ASSERT(set.count_leading_zeroes() == 30);

    set.clear();
    set.set(63);
    ASSERT(set.count_leading_zeroes() == 0);
    set.destroy();
}

// count_leading_ones.

intern void test_bitset_count_leading_ones(Allocator* a)
{
    DBitSet<u32> set;
    set.init(a, 64);
    ASSERT(set.count_leading_ones() == 0);

    set.set_all();
    ASSERT(set.count_leading_ones() == 64);

    set.clear();
    set.set(63);
    ASSERT(set.count_leading_ones() == 1);

    set.clear();
    set.set(62);
    set.set(63);
    ASSERT(set.count_leading_ones() == 2);

    set.clear();
    set.set(31);
    ASSERT(set.count_leading_ones() == 0);
    set.destroy();
}

// destroy.

intern void test_bitset_destroy(Allocator* a)
{
    DBitSet<u64> set;

    // Destroying an uninitialized bitset is a no-op.
    set.destroy();
    ASSERT(set.bit_capacity() == 0);

    set.init(a);
    set.set(4);
    set.destroy();
    ASSERT(set.bit_capacity() == 0);

    // Destroying twice is a no-op.
    set.destroy();
    set.destroy();
}

// bit_capacity.

intern void test_bitset_bit_capacity(Allocator* a)
{
    DBitSet<u64> set;
    ASSERT(set.bit_capacity() == 0);

    set.init(a);
    ASSERT(set.bit_capacity() == 64);

    set.init(a, 160);
    ASSERT(set.bit_capacity() == 192);
    set.destroy();
}

// is_all_set / is_any_set / is_nothing_set.

intern void test_bitset_states(Allocator* a)
{
    DBitSet<u64> set;
    set.init(a, 64);
    ASSERT(set.is_nothing_set());
    ASSERT(!set.is_any_set());
    ASSERT(!set.is_all_set());

    set.set(0);
    ASSERT(!set.is_nothing_set());
    ASSERT(set.is_any_set());
    ASSERT(!set.is_all_set());

    set.set_all();
    ASSERT(!set.is_nothing_set());
    ASSERT(set.is_any_set());
    ASSERT(set.is_all_set());
    set.destroy();
}

// calc_indices.

intern void test_bitset_calc_indices(Allocator* a)
{
    DBitSet<u64> set;
    set.init(a, 128);

    auto [bucket_0, bit_0] = set.calc_indices(0);
    ASSERT(bucket_0 == 0 && bit_0 == 0);

    auto [bucket_63, bit_63] = set.calc_indices(63);
    ASSERT(bucket_63 == 0 && bit_63 == 63);

    auto [bucket_64, bit_64] = set.calc_indices(64);
    ASSERT(bucket_64 == 1 && bit_64 == 0);

    auto [bucket_127, bit_127] = set.calc_indices(127);
    ASSERT(bucket_127 == 1 && bit_127 == 63);
    set.destroy();
}

void bitset_tests()
{
    HeapAlloc heap;
    heap.init();
    defer(heap.destroy());

    Arena* arena = Arena::create(&heap, 4096);
    defer(arena->destroy());

    Allocator* a = arena;

    LOG_TEST("DBitSet tests");

    test_bitset_init(a);
    test_bitset_set(a);
    test_bitset_unset(a);
    test_bitset_is_set(a);
    test_bitset_resize(a);
    test_bitset_set_all(a);
    test_bitset_clear(a);
    test_bitset_set_bit_count(a);
    test_bitset_unset_bit_count(a);
    test_bitset_find_first_zero_bit(a);
    test_bitset_find_first_set_bit(a);
    test_bitset_count_trailing_zeroes(a);
    test_bitset_count_trailing_ones(a);
    test_bitset_count_leading_zeroes(a);
    test_bitset_count_leading_ones(a);
    test_bitset_destroy(a);
    test_bitset_bit_capacity(a);
    test_bitset_states(a);
    test_bitset_calc_indices(a);
}

} // rg
