#ifndef _RG_RINGBUFFER_HPP_
#define _RG_RINGBUFFER_HPP_

#include "core/basic.hpp"
#include "core/atomic.hpp"
#include "collections/slice.hpp"

namespace rg
{

/*
 RingBuffer - FIFO, fixed capacity, thread-safe container.
 Capacity must be a power of two.
*/
template<typename Type, sz CAPACITY>
struct RingBuffer
{
    ASSERT_POW_OF_TWO_STATIC(CAPACITY);
    static constexpr sz DEFAULT_CAPACITY = 16;
 
    Type data[CAPACITY];
    Atomic<sz> read_idx;
    Atomic<sz> write_idx;

    RingBuffer();
    RingBuffer(const RingBuffer& rhs);
    RingBuffer& operator=(const RingBuffer& rhs);

    // Add item to the end.
    void push(const Type& val);
    // TODO:
    void push(Slice<Type> vals);
    // Remove item from the start.
    // Doesn't check if queue is empty, its assumed you checked it before call.
    Type pop();
    // Checks if queue is empty, returns empty result if it is.
    Maybe<Type> pop_safe();

    sz count() const;
    void clear()
    {
        this->read_idx.store(0);
        this->write_idx.store(0);
    }
    bool is_empty() const
    {
        return this->read_idx.load() == this->write_idx.load();
    }
    bool is_full() const
    {
        return this->write_idx.load() == rg::wrap_sub_assume_pow_two(this->read_idx.load(), sz(1), CAPACITY);
    }
};

template<typename Type, sz CAPACITY>
RingBuffer<Type, CAPACITY>::RingBuffer()
    : read_idx{0}, write_idx{0}
{}

template<typename Type, sz CAPACITY>
RingBuffer<Type, CAPACITY>::RingBuffer(const RingBuffer& rhs)
{
    mem_copy(this->data, rhs.data, sizeof(Type) * CAPACITY);
    this->read_idx.store(rhs.read_idx.load());
    this->write_idx.store(rhs.write_idx.load());
}

template<typename Type, sz CAPACITY>
RingBuffer<Type, CAPACITY>& RingBuffer<Type, CAPACITY>::operator=(const RingBuffer& rhs)
{
    mem_copy(this->data, rhs.data, sizeof(Type) * CAPACITY);
    this->read_idx.store(rhs.read_idx.load());
    this->write_idx.store(rhs.write_idx.load());
}

template<typename Type, sz CAPACITY>
void RingBuffer<Type, CAPACITY>::push(const Type& val)
{
    AtomicOrder ord = AtomicOrder::SEQ_CST;
    sz expected = this->write_idx.load(ord);
    sz curr_write;
    sz new_write;

    do
    {
        curr_write = expected;
        new_write = rg::wrap_add_assume_pow_two(expected, sz(1), CAPACITY); 
    } while (!this->write_idx.compare_exchange_weak(&expected, new_write, ord, ord));

    this->data[curr_write] = val;

    sz curr_read = this->read_idx.load(ord);
    if (new_write == curr_read)
    {
        curr_read = rg::wrap_add_assume_pow_two(curr_read, sz(1), CAPACITY);
        this->read_idx.store(curr_read);
    }
}

template<typename Type, sz CAPACITY>
Type RingBuffer<Type, CAPACITY>::pop()
{
    ASSERT_NON_EMPTY(this);

    AtomicOrder ord = AtomicOrder::SEQ_CST;
    sz expected = this->read_idx.load(ord);
    sz curr_read;
    sz new_read;

    do
    {
        curr_read = expected;
        new_read = rg::wrap_add_assume_pow_two(expected, sz(1), CAPACITY); 
    } while(!this->read_idx.compare_exchange_weak(&expected, new_read, ord));

    return this->data[curr_read];
}

template<typename Type, sz CAPACITY>
Maybe<Type> RingBuffer<Type, CAPACITY>::pop_safe()
{
    Maybe<Type> res;
    if (this->is_empty()) return res;

    AtomicOrder ord = AtomicOrder::SEQ_CST;
    sz expected = this->read_idx.load(ord);
    sz curr_read;
    sz new_read;

    do
    {
        curr_read = expected;
        new_read = rg::wrap_add_assume_pow_two(expected, sz(1), CAPACITY); 
    } while(!this->read_idx.compare_exchange_weak(&expected, new_read, ord));

    res.set_val(this->data[curr_read]);
    return res;
}

template<typename Type, sz CAPACITY>
sz RingBuffer<Type, CAPACITY>::count() const
{
    AtomicOrder ord = AtomicOrder::SEQ_CST;
    sz write = this->write_idx.load(ord);
    sz read = this->read_idx.load(ord);
    if (write < read)
    {
        return write + CAPACITY - read;
    }
    else return write - read;
}

} // rg

#endif // _RG_RINGBUFFER_HPP_
