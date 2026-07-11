#ifndef _RG_RINGBUFFER_HPP_
#define _RG_RINGBUFFER_HPP_

#include <atomic>
#include "core/basic.hpp"

namespace rg
{

/*
 Ringbuffer - FIFO, fixed capacity, thread-safe container.
 Capacity must be a power of two.
*/
template<typename Type, sz CAPACITY>
struct Ringbuffer
{
    ASSERT_POW_OF_TWO_STATIC(CAPACITY);
    static constexpr sz DEFAULT_CAPACITY = 16;
 
    Type data[CAPACITY];
    std::atomic<sz> read_idx;
    std::atomic<sz> write_idx;

    Ringbuffer();
    Ringbuffer(const Ringbuffer& rhs);
    Ringbuffer& operator=(const Ringbuffer& rhs);

    // Add item to the end.
    bool push(const Type& val);
    // Remove item from the start.
    bool pop(Type* out_val);

    sz count() const;
    void clear() { this->read_idx = 0; this->write_idx = 0; }
    bool is_empty() const { return this->read_idx == this->write_idx; }
    bool is_full() const
    {
        return this->write_idx == rg::wrap_sub_assume_pow_two(this->read_idx.load(), sz(1), CAPACITY);
    }
};

template<typename Type, sz CAPACITY>
Ringbuffer<Type, CAPACITY>::Ringbuffer()
    : read_idx{0}, write_idx{0}
{}

template<typename Type, sz CAPACITY>
Ringbuffer<Type, CAPACITY>::Ringbuffer(const Ringbuffer& rhs)
{
    mem_copy(this->data, rhs.data, sizeof(Type) * CAPACITY);
    this->read_idx.store(rhs.read_idx.load());
    this->write_idx.store(rhs.write_idx.load());
}

template<typename Type, sz CAPACITY>
Ringbuffer<Type, CAPACITY>& Ringbuffer<Type, CAPACITY>::operator=(const Ringbuffer& rhs)
{
    mem_copy(this->data, rhs.data, sizeof(Type) * CAPACITY);
    this->read_idx.store(rhs.read_idx.load());
    this->write_idx.store(rhs.write_idx.load());
}

template<typename Type, sz CAPACITY>
bool Ringbuffer<Type, CAPACITY>::push(const Type& val)
{
    if (this->is_full()) return false;

    sz expected = this->write_idx.load();
    sz curr_write;
    sz new_write;

    do
    {
        curr_write = expected;
        new_write = rg::wrap_add_assume_pow_two(expected, sz(1), CAPACITY); 
    } while(!this->write_idx.compare_exchange_weak(expected, new_write));

    this->data[curr_write] = val;

    sz curr_read = this->read_idx.load();
    if (new_write == curr_read)
    {
        curr_read = rg::wrap_add_assume_pow_two(curr_read, sz(1), CAPACITY);
        this->read_idx.store(curr_read);
    }

    return true;
}

template<typename Type, sz CAPACITY>
bool Ringbuffer<Type, CAPACITY>::pop(Type* out_val)
{
    if (this->is_empty()) return false;

    sz expected = this->read_idx.load();
    sz curr_read;
    sz new_read;

    do
    {
        curr_read = expected;
        new_read = rg::wrap_add_assume_pow_two(expected, sz(1), CAPACITY); 
    } while(!this->read_idx.compare_exchange_weak(expected, new_read));

    *out_val = this->data[curr_read];
    return true;
}

template<typename Type, sz CAPACITY>
sz Ringbuffer<Type, CAPACITY>::count() const
{
    sz write = this->write_idx.load();
    sz read = this->read_idx.load();
    if (write < read)
    {
        return write + CAPACITY - read;
    }
    else return write - read;
}

} // rg

#endif // _RG_RINGBUFFER_HPP_
