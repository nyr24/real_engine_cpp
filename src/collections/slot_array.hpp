#ifndef _RG_SLOT_ARRAY_HPP_
#define _RG_SLOT_ARRAY_HPP_

#include "core/basic.hpp"
#include "collections/bits.hpp"

namespace rg
{

/*
 SlotArray - array-based data structure, which allows for faster additions / removals at arbitrary places.
 Its using bitset for tracking free slots, to find them very fast.
*/
template<typename Type>
struct SlotArray
{
    static constexpr sz DEFAULT_CAPACITY = 16;

    // Iterates over active slots.
    struct Iter
    {
        Slice<Type> data_view;
        DBitSet<u64>* bits;
        sz pos;

        Maybe<Type*> next();

        void reset() { this->pos = 0; }
        bool at_end() { return this->pos == this->data_view.count; }
    };

    Type* data;
    sz capacity;
    DBitSet<u64> bits;
    Allocator* alloc;

    SlotArray();
    void init(Allocator* alloc, sz init_capacity = DEFAULT_CAPACITY);
    sz add(const Type& val);
    void add(Slice<Type> vals);
    void remove(sz idx);
    Type* get_free_slot();
    sz get_free_slot_idx();
    Pair<Type*, sz> get_free_slot_and_idx();
    Slice<Type*> get_free_slots(Allocator* alloc, sz count);
    Slice<sz> get_free_slot_idxs(Allocator* alloc, sz count);
    void resize(sz need_capacity);
    Iter get_active_slot_iter();
    void destroy();

    bool is_empty() { return this->bits.is_nothing_set(); }
    bool is_full() { return this->bits.is_all_set(); }
    bool is_initialized() { return this->alloc != null; }
    Type& operator[](sz idx) { return this->data[idx]; }
    Type* begin() { return this->data; };
    Type* end() { return this->data + capacity; };
};

template<typename Type>
SlotArray<Type>::SlotArray()
    : data{null}, capacity{}, alloc{null}
{
}

template<typename Type>
void SlotArray<Type>::init(Allocator* alloc, sz init_capacity)
{
    ASSERT_GREATER_ZERO(init_capacity);
    this->data = (Type*)allocator_allocate(alloc, init_capacity * sizeof(Type), alignof(Type));
    this->bits.init(alloc, init_capacity);
    this->capacity = init_capacity;
    this->alloc = alloc;
    this->bits.clear();
}

template<typename Type>
sz SlotArray<Type>::add(const Type& val)
{
    if (this->is_full())
    {
        sz free_slot = this->capacity;
        this->resize(1);
        this->data[free_slot] = val;
        this->bits.set(free_slot);
        return free_slot;
    }

    auto [idx, is_found] = this->bits.find_first_zero_bit(true);
    ASSERT_MSG(is_found, "Slot wasn't found after check on fullness");
    this->data[idx] = val;
    return idx;
}

template<typename Type>
void SlotArray<Type>::remove(sz idx)
{
    ASSERT_MSG(idx >= 0 && idx < this->capacity, "Must be in bounds");
    this->bits.unset(idx);
}

template<typename Type>
sz SlotArray<Type>::get_free_slot_idx()
{
    auto [idx, is_found] = this->bits.find_first_zero_bit(true);
    if (is_found) return idx;

    sz old_capacity = this->capacity;
    this->resize(old_capacity * 2);
    this->bits.set(old_capacity);
    return old_capacity;
}

template<typename Type>
Type* SlotArray<Type>::get_free_slot()
{
    sz idx = this->get_free_slot_idx();
    return this->data[idx];
}

template<typename Type>
Pair<Type*, sz> SlotArray<Type>::get_free_slot_and_idx()
{
    sz idx = this->get_free_slot_idx();
    return Pair{ &this->data[idx], idx };
}

template<typename Type>
Slice<Type*> SlotArray<Type>::get_free_slots(Allocator* alloc, sz count)
{
    ASSERT_GREATER_ZERO(count);

    auto res = Slice<Type*>::make(alloc, count);
    for (Type* slot : res)
    {
        *slot = this->get_free_slot();
    }
    return res;
}

template<typename Type>
Slice<sz> SlotArray<Type>::get_free_slot_idxs(Allocator* alloc, sz count)
{
    ASSERT_GREATER_ZERO(count);

    auto res = Slice<sz>::make(alloc, count);
    for (sz& slot : res)
    {
        slot = this->get_free_slot_idx();
    }
    return res;
}

template<typename Type>
void SlotArray<Type>::resize(sz needed)
{
    sz old_capacity = this->capacity;
    sz min_required = old_capacity + needed;
    sz new_capacity = old_capacity;

    if (new_capacity == 0) new_capacity = DEFAULT_CAPACITY;
    while (new_capacity < min_required)
    {
        new_capacity *= 2;
    }
    
    if (this->data) this->data = (Type*)allocator_reallocate(this->alloc, this->data, sizeof(Type) * new_capacity);
    else this->data = (Type*)allocator_allocate(this->alloc, sizeof(Type) * new_capacity, alignof(Type));

    this->bits.resize(new_capacity - old_capacity);
    this->capacity = new_capacity;
}

template<typename Type>
SlotArray<Type>::Iter SlotArray<Type>::get_active_slot_iter()
{
    SlotArray<Type>::Iter iter; 
    iter.data_view = { this->data, this->capacity };
    iter.pos = 0;
    iter.bits = &this->bits;
    return iter;
}

template<typename Type>
Maybe<Type*> SlotArray<Type>::Iter::next()
{
    Maybe<Type*> res;
    if (this->at_end()) return res;
    // We need the opposite of ctz.
    sz cto = this->bits->count_trailing_zeroes(this->pos);
    this->pos = cto;
    res.set_val(this->data_view[cto - 1]);
    return res;
}

template<typename Type>
void SlotArray<Type>::destroy()
{
    if (this->data)
    {
        allocator_free(this->alloc, this->data);
        this->data = null;
    }
}

} // rg

#endif // _RG_SLOT_ARRAY_HPP_
