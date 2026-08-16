#include "core/basic.hpp"
#include "core/allocators.hpp"
#include "collections/slot_array.hpp"
#include "collections/slice.hpp"

namespace rg
{

// Default ctor.

intern void test_slot_array_default_ctor()
{
    SlotArray<s32> sa;
    ASSERT(sa.data == null);
    ASSERT(sa.capacity == 0);
    ASSERT(sa.alloc == null);
    ASSERT(!sa.is_initialized());
    ASSERT(sa.is_empty());
}

// init.

intern void test_slot_array_init(Allocator* a)
{
    SlotArray<s32> sa;
    sa.init(a);
    ASSERT(sa.is_initialized());
    ASSERT(sa.capacity == SlotArray<s32>::DEFAULT_CAPACITY);
    ASSERT(sa.is_empty());
    ASSERT(!sa.is_full());

    SlotArray<s32> big;
    big.init(a, 64);
    ASSERT(big.capacity == 64);
    ASSERT(big.is_empty());
    big.destroy();
    sa.destroy();
}

// add.

intern void test_slot_array_add(Allocator* a)
{
    SlotArray<s32> sa;
    sa.init(a);

    ASSERT(sa.add(100) == 0);
    ASSERT(sa.add(200) == 1);
    ASSERT(sa.add(300) == 2);
    ASSERT(sa[0] == 100);
    ASSERT(sa[1] == 200);
    ASSERT(sa[2] == 300);
    ASSERT(!sa.is_empty());

    // Fill up to capacity.
    for (s32 i = 3; i < 16; ++i)
    {
        sa.add(i);
    }
    ASSERT(sa.is_full());

    // Adding past capacity grows the array.
    sz idx = sa.add(16);
    ASSERT(idx == 16);
    ASSERT(sa.capacity > 16);
    ASSERT(!sa.is_full());
    ASSERT(sa[16] == 16);
    sa.destroy();
}

// add(Slice).

intern void test_slot_array_add_slice(Allocator* a)
{
    SlotArray<s32> sa;
    sa.init(a);

    s32 vals[3] = { 10, 20, 30 };
    Slice<s32> slice = { vals, 3 };
    sa.add(slice);
    ASSERT(sa[0] == 10);
    ASSERT(sa[1] == 20);
    ASSERT(sa[2] == 30);
    ASSERT(!sa.is_empty());
    sa.destroy();
}

// remove.

intern void test_slot_array_remove(Allocator* a)
{
    SlotArray<s32> sa;
    sa.init(a);
    sa.add(1);
    sa.add(2);
    sa.add(3);

    sa.remove(1);
    ASSERT(!sa.is_empty());
    // Removed slot gets reused by the next add.
    ASSERT(sa.add(4) == 1);
    ASSERT(sa[1] == 4);

    sa.remove(0);
    sa.remove(1);
    sa.remove(2);
    ASSERT(sa.is_empty());
    sa.destroy();
}

// get_free_slot.

intern void test_slot_array_get_free_slot(Allocator* a)
{
    SlotArray<s32> sa;
    sa.init(a);

    s32* slot = sa.get_free_slot();
    ASSERT(slot == &sa[0]);
    *slot = 5;
    ASSERT(sa[0] == 5);
    // Slot 0 is now taken.
    s32* next = sa.get_free_slot();
    ASSERT(next == &sa[1]);

    sa.destroy();
}

// get_free_slot_idx.

intern void test_slot_array_get_free_slot_idx(Allocator* a)
{
    SlotArray<s32> sa;
    sa.init(a);
    ASSERT(sa.get_free_slot_idx() == 0);
    ASSERT(sa.get_free_slot_idx() == 1);
    ASSERT(sa.get_free_slot_idx() == 2);

    // After removing a slot, its index is handed out again.
    sa.remove(1);
    ASSERT(sa.get_free_slot_idx() == 1);

    // On a full array it grows and hands out the first new slot.
    while (!sa.is_full())
    {
        sa.get_free_slot_idx();
    }
    sz grow_idx = sa.get_free_slot_idx();
    ASSERT(grow_idx == 16);
    ASSERT(sa.capacity > 16);
    sa.destroy();
}

// get_free_slot_and_idx.

intern void test_slot_array_get_free_slot_and_idx(Allocator* a)
{
    SlotArray<s32> sa;
    sa.init(a);

    Pair<s32*, sz> pair = sa.get_free_slot_and_idx();
    ASSERT(pair.second == 0);
    ASSERT(pair.first == &sa[0]);
    *pair.first = 9;
    ASSERT(sa[0] == 9);

    sa.remove(0);
    pair = sa.get_free_slot_and_idx();
    ASSERT(pair.second == 0);
    sa.destroy();
}

// get_free_slots.

intern void test_slot_array_get_free_slots(Allocator* a)
{
    SlotArray<s32> sa;
    sa.init(a);

    Slice<s32*> slots = sa.get_free_slots(a, 3);
    ASSERT(slots.count == 3);
    ASSERT(slots[0] == &sa[0]);
    ASSERT(slots[1] == &sa[1]);
    ASSERT(slots[2] == &sa[2]);
    *slots[0] = 1;
    *slots[1] = 2;
    *slots[2] = 3;
    ASSERT(sa[0] == 1 && sa[1] == 2 && sa[2] == 3);
    // The slots are now occupied.
    ASSERT(sa.add(4) == 3);
    sa.destroy();
}

// get_free_slot_idxs.

intern void test_slot_array_get_free_slot_idxs(Allocator* a)
{
    SlotArray<s32> sa;
    sa.init(a);

    Slice<sz> idxs = sa.get_free_slot_idxs(a, 3);
    ASSERT(idxs.count == 3);
    ASSERT(idxs[0] == 0);
    ASSERT(idxs[1] == 1);
    ASSERT(idxs[2] == 2);
    ASSERT(sa.add(0) == 3);
    sa.destroy();
}

// resize.

intern void test_slot_array_resize(Allocator* a)
{
    SlotArray<s32> sa;
    sa.init(a);
    sa.add(10);
    sa.add(20);
    sa.add(30);

    sz old_capacity = sa.capacity;
    sa.resize(1);
    ASSERT(sa.capacity == old_capacity * 2);
    // Existing values are preserved.
    ASSERT(sa[0] == 10);
    ASSERT(sa[1] == 20);
    ASSERT(sa[2] == 30);
    // Existing slots are still active.
    ASSERT(sa.add(40) == 3);
    sa.destroy();
}

// get_active_slot_iter.

intern void test_slot_array_iter(Allocator* a)
{
    SlotArray<s32> sa;
    sa.init(a);
    sa.add(100);
    sa.add(200);
    sa.add(300);
    sa.remove(1);

    SlotArray<s32>::Iter iter = sa.get_active_slot_iter();
    ASSERT(iter.data_view.count == sa.capacity);

    Maybe<s32*> first = iter.next();
    ASSERT(first.is_ok);
    ASSERT(*first.val == 100);
    ASSERT(iter.pos == 1);

    Maybe<s32*> second = iter.next();
    ASSERT(second.is_ok);
    ASSERT(*second.val == 300);
    ASSERT(iter.pos == 3);

    ASSERT(!iter.at_end());
    Maybe<s32*> end = iter.next();
    ASSERT(!end.is_ok);
    ASSERT(iter.at_end());

    // reset restarts from the beginning.
    iter.reset();
    Maybe<s32*> again = iter.next();
    ASSERT(again.is_ok);
    ASSERT(*again.val == 100);
    sa.destroy();
}

// destroy.

intern void test_slot_array_destroy(Allocator* a)
{
    SlotArray<s32> sa;
    sa.init(a);
    sa.add(1);

    sa.destroy();
    ASSERT(sa.data == null);

    // Destroying twice is a no-op.
    sa.destroy();
}

// operator[].

intern void test_slot_array_operator_index(Allocator* a)
{
    SlotArray<s32> sa;
    sa.init(a);
    sz idx = sa.add(7);
    ASSERT(sa[idx] == 7);
    sa[idx] = 8;
    ASSERT(sa[idx] == 8);
    sa.destroy();
}

// begin / end.

intern void test_slot_array_begin_end(Allocator* a)
{
    SlotArray<s32> sa;
    sa.init(a);
    ASSERT(sa.begin() == sa.data);
    ASSERT(sa.end() == sa.data + sa.capacity);
    sa.destroy();
}

// is_empty / is_full / is_initialized.

intern void test_slot_array_states(Allocator* a)
{
    SlotArray<s32> sa;
    ASSERT(!sa.is_initialized());

    sa.init(a);
    ASSERT(sa.is_initialized());
    ASSERT(sa.is_empty());
    ASSERT(!sa.is_full());

    for (sz i = 0; i < sa.capacity; ++i)
    {
        sa.add(i);
    }
    ASSERT(!sa.is_empty());
    ASSERT(sa.is_full());

    sa.remove(0);
    ASSERT(!sa.is_full());
    sa.destroy();
}

void slot_array_tests()
{
    HeapAlloc heap;
    heap.init();
    defer(heap.destroy());

    Arena* arena = Arena::create(&heap, 8192);
    defer(arena->destroy());

    Allocator* a = arena;

    LOG_TEST("SlotArray tests");

    test_slot_array_default_ctor();
    test_slot_array_init(a);
    test_slot_array_add(a);
    test_slot_array_add_slice(a);
    test_slot_array_remove(a);
    test_slot_array_get_free_slot(a);
    test_slot_array_get_free_slot_idx(a);
    test_slot_array_get_free_slot_and_idx(a);
    test_slot_array_get_free_slots(a);
    test_slot_array_get_free_slot_idxs(a);
    test_slot_array_resize(a);
    test_slot_array_iter(a);
    test_slot_array_destroy(a);
    test_slot_array_operator_index(a);
    test_slot_array_begin_end(a);
    test_slot_array_states(a);
}

} // rg
