#include "core/basic.hpp"
#include "core/allocators.hpp"
#include "collections/ringbuffer.hpp"
#include "collections/slice.hpp"

namespace rg
{

// Shared callback state.

intern s64 g_rb_sum;

intern void rb_accum(const s32& val)
{
    g_rb_sum += val;
}

intern void rb_accum_ref(s32* val)
{
    g_rb_sum += *val;
}

intern void rb_add_ten(s32* val)
{
    *val += 10;
}

// Default ctor.

intern void test_ringbuffer_default_ctor()
{
    RingBuffer<s32, 16> rb;
    ASSERT(rb.read_idx == 0);
    ASSERT(rb.write_idx == 0);
    ASSERT(rb.is_empty());
    ASSERT(!rb.is_full());
    ASSERT(rb.count() == 0);
}

// Copy ctor.

intern void test_ringbuffer_copy_ctor()
{
    RingBuffer<s32, 16> rb;
    rb.push(1);
    rb.push(2);
    rb.push(3);

    RingBuffer<s32, 16> copy(rb);
    ASSERT(copy.count() == rb.count());
    ASSERT(copy.pop() == 1);
    ASSERT(copy.pop() == 2);
    ASSERT(copy.pop() == 3);
    ASSERT(copy.is_empty());
    // Original is unaffected by the copy's pops.
    ASSERT(rb.count() == 3);
    ASSERT(rb.pop() == 1);
}

// Copy assignment.

intern void test_ringbuffer_copy_assign()
{
    RingBuffer<s32, 16> src;
    src.push(10);
    src.push(20);

    RingBuffer<s32, 16> dst;
    dst.push(99);
    dst = src;
    ASSERT(dst.count() == src.count());
    ASSERT(dst.pop() == 10);
    ASSERT(dst.pop() == 20);
    ASSERT(dst.is_empty());
    // Original is unaffected by the copy's pops.
    ASSERT(src.count() == 2);
}

// push.

intern void test_ringbuffer_push()
{
    RingBuffer<s32, 16> rb;
    for (s32 i = 1; i <= 15; ++i)
    {
        rb.push(i);
    }
    ASSERT(rb.count() == 15);
    ASSERT(rb.is_full());

    // Pushing while full drops the oldest item.
    rb.push(16);
    ASSERT(rb.count() == 15);
    ASSERT(rb.is_full());
    ASSERT(rb.pop() == 2);
    ASSERT(rb.count() == 14);
}

// push(Slice).

intern void test_ringbuffer_push_slice()
{
    s32 vals[4] = { 5, 6, 7, 8 };
    Slice<s32> slice = { vals, 4 };

    RingBuffer<s32, 16> rb;
    rb.push(slice);
    ASSERT(rb.count() == 4);
    ASSERT(rb.pop() == 5);
    ASSERT(rb.pop() == 6);
    ASSERT(rb.pop() == 7);
    ASSERT(rb.pop() == 8);
    ASSERT(rb.is_empty());
}

// pop.

intern void test_ringbuffer_pop()
{
    RingBuffer<s32, 16> rb;
    rb.push(10);
    rb.push(20);
    ASSERT(rb.pop() == 10);
    ASSERT(rb.pop() == 20);
    ASSERT(rb.is_empty());

    // Popping right after a full wrap.
    for (s32 i = 1; i <= 15; ++i)
    {
        rb.push(i);
    }
    ASSERT(rb.pop() == 1);
}

// pop_safe.

intern void test_ringbuffer_pop_safe()
{
    RingBuffer<s32, 16> rb;
    auto missing = rb.pop_safe();
    ASSERT(!missing.is_ok);

    rb.push(42);
    auto found = rb.pop_safe();
    ASSERT(found.is_ok);
    ASSERT(found.val == 42);

    auto again = rb.pop_safe();
    ASSERT(!again.is_ok);
}

// foreach.

intern void test_ringbuffer_foreach()
{
    RingBuffer<s32, 16> rb;
    for (s32 i = 1; i <= 5; ++i)
    {
        rb.push(i);
    }

    g_rb_sum = 0;
    rb.foreach(rb_accum);
    ASSERT(g_rb_sum == 15);
    // foreach consumes the buffer.
    ASSERT(rb.is_empty());
}

// foreach_ref.

intern void test_ringbuffer_foreach_ref()
{
    RingBuffer<s32, 16> rb;
    for (s32 i = 1; i <= 5; ++i)
    {
        rb.push(i);
    }

    g_rb_sum = 0;
    rb.foreach_ref(rb_accum_ref);
    ASSERT(g_rb_sum == 15);
    ASSERT(rb.is_empty());

    // Mutating through foreach_ref only affects the local copy.
    RingBuffer<s32, 16> rb2;
    rb2.push(7);
    rb2.foreach_ref(rb_add_ten);
    ASSERT(rb2.is_empty());
}

// count.

intern void test_ringbuffer_count()
{
    RingBuffer<s32, 16> rb;
    ASSERT(rb.count() == 0);
    rb.push(1);
    rb.push(2);
    rb.push(3);
    ASSERT(rb.count() == 3);
    rb.pop();
    ASSERT(rb.count() == 2);
    rb.clear();
    ASSERT(rb.count() == 0);
}

// clear.

intern void test_ringbuffer_clear()
{
    RingBuffer<s32, 16> rb;
    rb.push(1);
    rb.push(2);
    rb.clear();
    ASSERT(rb.is_empty());
    ASSERT(rb.count() == 0);
    ASSERT(rb.read_idx == 0);
    ASSERT(rb.write_idx == 0);
}

// is_empty.

intern void test_ringbuffer_is_empty()
{
    RingBuffer<s32, 16> rb;
    ASSERT(rb.is_empty());
    rb.push(1);
    ASSERT(!rb.is_empty());
    rb.pop();
    ASSERT(rb.is_empty());
}

// is_full.

intern void test_ringbuffer_is_full()
{
    RingBuffer<s32, 16> rb;
    ASSERT(!rb.is_full());
    for (s32 i = 0; i < 15; ++i)
    {
        rb.push(i);
    }
    ASSERT(rb.is_full());
    ASSERT(rb.count() == 15);
    rb.pop();
    ASSERT(!rb.is_full());
    ASSERT(rb.count() == 14);
}

void ringbuffer_tests()
{
    LOG_TEST("RingBuffer tests");

    test_ringbuffer_default_ctor();
    test_ringbuffer_copy_ctor();
    test_ringbuffer_copy_assign();
    test_ringbuffer_push();
    test_ringbuffer_push_slice();
    test_ringbuffer_pop();
    test_ringbuffer_pop_safe();
    test_ringbuffer_foreach();
    test_ringbuffer_foreach_ref();
    test_ringbuffer_count();
    test_ringbuffer_clear();
    test_ringbuffer_is_empty();
    test_ringbuffer_is_full();
}

} // rg
