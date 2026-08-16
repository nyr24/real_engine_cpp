#include "core/basic.hpp"
#include "core/allocators.hpp"
#include "collections/hashmap.hpp"
#include "collections/slice.hpp"

namespace rg
{

// Simple hashable key type for tests.

struct TestKey
{
    s32 id;

    u64 hash() const
    {
        u64 h = FNV_OFFSET_BASIS;
        h ^= u64(this->id);
        h *= FNV_PRIME;
        return h;
    }
};

bool operator==(const TestKey& lhs, const TestKey& rhs)
{
    return lhs.id == rhs.id;
}

// Shared counters for foreach callbacks.

intern s32 g_foreach_count;
intern s64 g_value_sum;

intern void count_pair(const Pair<TestKey*, s32*>&) { g_foreach_count++; }
intern void mutate_pair(Pair<TestKey*, s32*> pair) { *pair.second = 42; }
intern void count_key(const TestKey&) { g_foreach_count++; }
intern void count_key_mut(TestKey*) { g_foreach_count++; }
intern void sum_value(const s32& val) { g_value_sum += val; }
intern void set_value(s32* val) { *val = 7; }

// Default ctor.

intern void test_hashmap_default_ctor()
{
    HashMap<TestKey, s32> map;
    ASSERT(map.data == null);
    ASSERT(map.alloc == null);
    ASSERT(map.count == 0);
    ASSERT(map.capacity == 0);
    ASSERT(!map.is_initialized());
    ASSERT(map.is_empty());
    ASSERT(map.len() == 0);
}

// init.

intern void test_hashmap_init(Allocator* a)
{
    HashMap<TestKey, s32> map;
    map.init(a);
    ASSERT(map.is_initialized());
    ASSERT(map.capacity == (HashMap<TestKey, s32>::DEFAULT_CAPACITY));
    ASSERT(map.len() == 0);
    ASSERT(map.is_empty());
    map.destroy();

    HashMap<TestKey, s32> big;
    big.init(a, 100);
    ASSERT(big.is_initialized());
    ASSERT(big.capacity == 128);
    big.destroy();
}

// init_with_key_values.

intern void test_hashmap_init_with_key_values(Allocator* a)
{
    TestKey k1{1};
    TestKey k2{2};
    TestKey k3{3};
    HashMapKVPair<TestKey, s32> pairs[3] = {
        { k1, 10 },
        { k2, 20 },
        { k3, 30 },
    };
    Slice<HashMapKVPair<TestKey, s32>> pairs_slice = { pairs, 3 };

    HashMap<TestKey, s32> map;
    map.init_with_key_values(a, pairs_slice);
    ASSERT(map.is_initialized());
    ASSERT(map.len() == 3);
    ASSERT(map.capacity == 16);

    auto got = map.get(k2);
    ASSERT(got.is_ok);
    ASSERT(*(got.val) == 20);
    map.destroy();
}

// put.

intern void test_hashmap_put(Allocator* a)
{
    HashMap<TestKey, s32> map;
    map.init(a, 16);

    for (s32 i = 0; i < 100; ++i)
    {
        map.put(TestKey{i}, i * 10);
    }

    ASSERT(map.len() == 100);
    ASSERT(map.capacity == 128);

    for (s32 i = 0; i < 100; ++i)
    {
        auto got = map.get(TestKey{i});
        ASSERT(got.is_ok);
        ASSERT(*(got.val) == i * 10);
    }
    map.destroy();
}

// get.

intern void test_hashmap_get(Allocator* a)
{
    HashMap<TestKey, s32> map;
    map.init(a);

    auto missing = map.get(TestKey{42});
    ASSERT(!missing.is_ok);

    map.put(TestKey{42}, 777);
    auto found = map.get(TestKey{42});
    ASSERT(found.is_ok);
    ASSERT(*(found.val) == 777);

    map.destroy();
}

// has.

intern void test_hashmap_has(Allocator* a)
{
    HashMap<TestKey, s32> map;
    map.init(a);
    ASSERT(!map.has(TestKey{1}));

    map.put(TestKey{1}, 100);
    ASSERT(map.has(TestKey{1}));

    map.remove(TestKey{1});
    ASSERT(!map.has(TestKey{1}));
    map.destroy();
}

// remove.

intern void test_hashmap_remove(Allocator* a)
{
    HashMap<TestKey, s32> map;
    map.init(a);
    ASSERT(!map.remove(TestKey{1}));

    map.put(TestKey{1}, 100);
    map.put(TestKey{2}, 200);

    ASSERT(map.remove(TestKey{1}));
    ASSERT(!map.has(TestKey{1}));
    ASSERT(map.has(TestKey{2}));
    ASSERT(map.len() == 1);

    ASSERT(!map.remove(TestKey{1}));
    ASSERT(map.len() == 1);
    map.destroy();
}

// destroy.

intern void test_hashmap_destroy(Allocator* a)
{
    HashMap<TestKey, s32> map;
    map.init(a);
    map.put(TestKey{1}, 1);
    map.destroy();
    ASSERT(map.data == null);
    ASSERT(map.count == 0);
    ASSERT(map.capacity == 0);
    ASSERT(map.len() == 0);

    // Destroying twice is a no-op.
    map.destroy();
}

// get_iter.

intern void test_hashmap_iter(Allocator* a)
{
    HashMap<TestKey, s32> map;
    map.init(a, 16);
    map.put(TestKey{1}, 100);

    auto iter = map.get_iter();
    ASSERT(!iter.is_at_end());
    iter.reset();
    ASSERT(!iter.is_at_end());

    sz pairs = 0;
    for (;;)
    {
        Pair<TestKey*, s32*> pair = iter.next_pair();
        if (!pair.first || !pair.second) break;
        pairs++;
    }
    ASSERT(pairs == map.capacity);
    ASSERT(iter.is_at_end());

    iter.reset();
    sz keys = 0;
    for (;;)
    {
        TestKey* key = iter.next_key();
        if (!key) break;
        keys++;
    }
    ASSERT(keys == map.capacity);

    iter.reset();
    sz values = 0;
    for (;;)
    {
        s32* value = iter.next_value();
        if (!value) break;
        values++;
    }
    ASSERT(values == map.capacity);

    map.destroy();
}

// foreach_pair.

intern void test_hashmap_foreach_pair(Allocator* a)
{
    HashMap<TestKey, s32> map;
    map.init(a, 16);
    map.put(TestKey{1}, 100);
    map.put(TestKey{2}, 200);

    g_foreach_count = 0;
    map.foreach_pair(count_pair);
    ASSERT(g_foreach_count == map.capacity);
    map.destroy();
}

// foreach_pair_mut.

intern void test_hashmap_foreach_pair_mut(Allocator* a)
{
    HashMap<TestKey, s32> map;
    map.init(a, 16);
    map.put(TestKey{1}, 100);
    map.put(TestKey{2}, 200);

    map.foreach_pair_mut(mutate_pair);

    auto got = map.get(TestKey{1});
    ASSERT(got.is_ok);
    ASSERT(*(got.val) == 42);
    map.destroy();
}

// foreach_key.

intern void test_hashmap_foreach_key(Allocator* a)
{
    HashMap<TestKey, s32> map;
    map.init(a, 16);
    map.put(TestKey{1}, 100);

    g_foreach_count = 0;
    map.foreach_key(count_key);
    ASSERT(g_foreach_count == map.capacity);
    map.destroy();
}

// foreach_key_mut.

intern void test_hashmap_foreach_key_mut(Allocator* a)
{
    HashMap<TestKey, s32> map;
    map.init(a, 16);
    map.put(TestKey{1}, 100);

    g_foreach_count = 0;
    map.foreach_key_mut(count_key_mut);
    ASSERT(g_foreach_count == map.capacity);
    map.destroy();
}

// foreach_value.

intern void test_hashmap_foreach_value(Allocator* a)
{
    HashMap<TestKey, s32> map;
    map.init(a, 16);
    map.put(TestKey{1}, 100);

    // Make every slot hold the same value, so a full iteration is predictable.
    map.foreach_value_mut(set_value);

    g_value_sum = 0;
    map.foreach_value(sum_value);
    ASSERT(g_value_sum == map.capacity * 7);

    auto got = map.get(TestKey{1});
    ASSERT(got.is_ok);
    ASSERT(*(got.val) == 7);
    map.destroy();
}

// clone.

intern void test_hashmap_clone(Allocator* a)
{
    HashMap<TestKey, s32> map;
    map.init(a, 16);
    map.put(TestKey{1}, 100);
    map.put(TestKey{2}, 200);

    HashMap<TestKey, s32> cl = map.clone();
    ASSERT(cl.is_initialized());
    ASSERT(cl.len() == map.len());
    ASSERT(cl.capacity == map.capacity);

    auto got = cl.get(TestKey{2});
    ASSERT(got.is_ok);
    ASSERT(*(got.val) == 200);

    cl.destroy();
    map.destroy();
}

// Move ctor.

intern void test_hashmap_move_ctor(Allocator* a)
{
    HashMap<TestKey, s32> src;
    src.init(a, 16);
    src.put(TestKey{1}, 100);

    HashMap<TestKey, s32> moved(rg::move(src));
    ASSERT(moved.is_initialized());
    ASSERT(moved.len() == 1);
    ASSERT(moved.capacity == src.capacity || src.capacity == 0);

    auto got = moved.get(TestKey{1});
    ASSERT(got.is_ok);
    ASSERT(*(got.val) == 100);

    ASSERT(!src.is_initialized());
    ASSERT(src.data == null);
    ASSERT(src.capacity == 0);

    moved.destroy();
}

// Move assignment.

intern void test_hashmap_move_assign(Allocator* a)
{
    HashMap<TestKey, s32> dst;
    dst.init(a, 16);
    dst.put(TestKey{9}, 900);

    HashMap<TestKey, s32> src;
    src.init(a, 16);
    src.put(TestKey{1}, 100);

    dst = rg::move(src);
    ASSERT(dst.is_initialized());
    ASSERT(dst.len() == 1);

    auto got = dst.get(TestKey{1});
    ASSERT(got.is_ok);
    ASSERT(*(got.val) == 100);

    ASSERT(!src.is_initialized());
    ASSERT(src.data == null);

    dst.destroy();
}

// Copy ctor.

intern void test_hashmap_copy_ctor(Allocator* a)
{
    HashMap<TestKey, s32> src;
    src.init(a, 16);
    src.put(TestKey{1}, 100);

    HashMap<TestKey, s32> copy(src);
    ASSERT(copy.is_initialized());
    ASSERT(copy.len() == src.len());
    ASSERT(copy.capacity == src.capacity);
    ASSERT(copy.data == src.data);

    auto got = copy.get(TestKey{1});
    ASSERT(got.is_ok);
    ASSERT(*(got.val) == 100);

    copy.destroy();
    src.destroy();
}

// Copy assignment.

intern void test_hashmap_copy_assign(Allocator* a)
{
    HashMap<TestKey, s32> lhs;
    lhs.init(a, 16);
    lhs.put(TestKey{1}, 100);

    HashMap<TestKey, s32> rhs;
    rhs.init(a, 16);
    rhs.put(TestKey{2}, 200);

    lhs = rhs;
    ASSERT(lhs.len() == rhs.len());
    ASSERT(lhs.capacity == rhs.capacity);
    ASSERT(lhs.data == rhs.data);

    auto got = lhs.get(TestKey{2});
    ASSERT(got.is_ok);
    ASSERT(*(got.val) == 200);

    lhs.destroy();
    rhs.destroy();
}

// len / is_empty / is_initialized / byte_size.

intern void test_hashmap_sizes(Allocator* a)
{
    HashMap<TestKey, s32> map;
    ASSERT(map.len() == 0);
    ASSERT(map.is_empty());
    ASSERT(!map.is_initialized());

    map.init(a, 16);
    ASSERT(map.is_initialized());
    ASSERT(map.is_empty());
    ASSERT(map.len() == 0);

    map.put(TestKey{1}, 100);
    ASSERT(map.len() == 1);
    ASSERT(!map.is_empty());

    const sz slot_size = sizeof(u64) + sizeof(TestKey) + sizeof(s32);
    ASSERT(map.byte_size_used() == slot_size);
    ASSERT(map.byte_size_allocated() == map.capacity * slot_size);

    map.destroy();
}

void hashmap_tests()
{
    HeapAlloc heap;
    heap.init();
    defer(heap.destroy());

    Arena* arena = Arena::create(&heap, 8192);
    defer(arena->destroy());

    Allocator* a = arena;

    LOG_TEST("HashMap tests");

    test_hashmap_default_ctor();
    test_hashmap_init(a);
    test_hashmap_init_with_key_values(a);
    test_hashmap_put(a);
    test_hashmap_get(a);
    test_hashmap_has(a);
    test_hashmap_remove(a);
    test_hashmap_destroy(a);
    test_hashmap_iter(a);
    test_hashmap_foreach_pair(a);
    test_hashmap_foreach_pair_mut(a);
    test_hashmap_foreach_key(a);
    test_hashmap_foreach_key_mut(a);
    test_hashmap_foreach_value(a);
    test_hashmap_clone(a);
    test_hashmap_move_ctor(a);
    test_hashmap_move_assign(a);
    test_hashmap_copy_ctor(a);
    test_hashmap_copy_assign(a);
    test_hashmap_sizes(a);
}

} // rg
