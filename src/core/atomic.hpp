#ifndef _RG_ATOMIC_HPP_
#define _RG_ATOMIC_HPP_

#include "core/basic.hpp"

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace rg
{

enum struct AtomicOrder : s32
{
    RELAXED = 0,
    CONSUME = 1,
    ACQUIRE = 2,
    RELEASE = 3,
    ACQ_REL = 4,
    SEQ_CST = 5 
};

template<typename Type>
struct Atomic
{
    Type val;

    Type load(AtomicOrder order = AtomicOrder::SEQ_CST) const;
    void store(Type new_val, AtomicOrder order = AtomicOrder::SEQ_CST);
    void add(Type rhs, AtomicOrder order = AtomicOrder::SEQ_CST);
    void sub(Type rhs, AtomicOrder order = AtomicOrder::SEQ_CST);
    void mul(Type rhs, AtomicOrder order = AtomicOrder::SEQ_CST);
    void div(Type rhs, AtomicOrder order = AtomicOrder::SEQ_CST);
    void bit_and(Type rhs, AtomicOrder order = AtomicOrder::SEQ_CST);
    void bit_xor(Type rhs, AtomicOrder order = AtomicOrder::SEQ_CST);
    void bit_or(Type rhs, AtomicOrder order = AtomicOrder::SEQ_CST);
    Type exchange(Type rhs, AtomicOrder order = AtomicOrder::SEQ_CST);
    bool compare_exchange_weak(Type* expected_val, Type new_val, AtomicOrder success_order = AtomicOrder::SEQ_CST, AtomicOrder fail_order = AtomicOrder::SEQ_CST);
    bool compare_exchange_strong(Type* expected_val, Type new_val, AtomicOrder success_order = AtomicOrder::SEQ_CST, AtomicOrder fail_order = AtomicOrder::SEQ_CST);
};

#ifdef _MSC_VER

namespace detail
{

template<sz SIZE> struct Interlocked;

template<> struct Interlocked<1>
{
    using Type = char;
    static Type load(volatile Type* p) { return (Type)_InterlockedOr8((volatile char*)p, 0); }
    static void store(volatile Type* p, Type v) { _InterlockedExchange8((volatile char*)p, (char)v); }
    static Type fetch_add(volatile Type* p, Type v) { return (Type)_InterlockedExchangeAdd8((volatile char*)p, (char)v); }
    static Type bit_and(volatile Type* p, Type v) { return (Type)_InterlockedAnd8((volatile char*)p, (char)v); }
    static Type bit_or(volatile Type* p, Type v) { return (Type)_InterlockedOr8((volatile char*)p, (char)v); }
    static Type bit_xor(volatile Type* p, Type v) { return (Type)_InterlockedXor8((volatile char*)p, (char)v); }
    static Type exchange(volatile Type* p, Type v) { return (Type)_InterlockedExchange8((volatile char*)p, (char)v); }
    static Type compare_exchange(volatile Type* p, Type x, Type c) { return (Type)_InterlockedCompareExchange8((volatile char*)p, (char)x, (char)c); }
};

template<> struct Interlocked<2>
{
    using Type = short;
    static Type load(volatile Type* p) { return (Type)_InterlockedOr16((volatile short*)p, 0); }
    static void store(volatile Type* p, Type v) { _InterlockedExchange16((volatile short*)p, (short)v); }
    static Type fetch_add(volatile Type* p, Type v) { return (Type)_InterlockedExchangeAdd16((volatile short*)p, (short)v); }
    static Type bit_and(volatile Type* p, Type v) { return (Type)_InterlockedAnd16((volatile short*)p, (short)v); }
    static Type bit_or(volatile Type* p, Type v) { return (Type)_InterlockedOr16((volatile short*)p, (short)v); }
    static Type bit_xor(volatile Type* p, Type v) { return (Type)_InterlockedXor16((volatile short*)p, (short)v); }
    static Type exchange(volatile Type* p, Type v) { return (Type)_InterlockedExchange16((volatile short*)p, (short)v); }
    static Type compare_exchange(volatile Type* p, Type x, Type c) { return (Type)_InterlockedCompareExchange16((volatile short*)p, (short)x, (short)c); }
};

template<> struct Interlocked<4>
{
    using Type = long;
    static Type load(volatile Type* p) { return _InterlockedOr(p, 0); }
    static void store(volatile Type* p, Type v) { _InterlockedExchange(p, v); }
    static Type fetch_add(volatile Type* p, Type v) { return _InterlockedExchangeAdd(p, v); }
    static Type bit_and(volatile Type* p, Type v) { return _InterlockedAnd(p, v); }
    static Type bit_or(volatile Type* p, Type v) { return _InterlockedOr(p, v); }
    static Type bit_xor(volatile Type* p, Type v) { return _InterlockedXor(p, v); }
    static Type exchange(volatile Type* p, Type v) { return _InterlockedExchange(p, v); }
    static Type compare_exchange(volatile Type* p, Type x, Type c) { return _InterlockedCompareExchange(p, x, c); }
};

template<> struct Interlocked<8>
{
    using Type = __int64;
    static Type load(volatile Type* p) { return _InterlockedOr64(p, 0); }
    static void store(volatile Type* p, Type v) { _InterlockedExchange64(p, v); }
    static Type fetch_add(volatile Type* p, Type v) { return _InterlockedExchangeAdd64(p, v); }
    static Type bit_and(volatile Type* p, Type v) { return _InterlockedAnd64(p, v); }
    static Type bit_or(volatile Type* p, Type v) { return _InterlockedOr64(p, v); }
    static Type bit_xor(volatile Type* p, Type v) { return _InterlockedXor64(p, v); }
    static Type exchange(volatile Type* p, Type v) { return _InterlockedExchange64(p, v); }
    static Type compare_exchange(volatile Type* p, Type x, Type c) { return _InterlockedCompareExchange64(p, x, c); }
};

} // detail

template<typename Type>
Type Atomic<Type>::load(AtomicOrder order) const
{
    using Ops = detail::Interlocked<sizeof(Type)>;
    return (Type)Ops::load((volatile typename Ops::Type*)&this->val); 
}

template<typename Type>
void Atomic<Type>::store(Type new_val, AtomicOrder order)
{
    using Ops = detail::Interlocked<sizeof(Type)>;
    Ops::store((volatile typename Ops::Type*)&this->val, (typename Ops::Type)new_val); 
}

template<typename Type>
void Atomic<Type>::add(Type rhs, AtomicOrder order)
{
    using Ops = detail::Interlocked<sizeof(Type)>;
    Ops::fetch_add((volatile typename Ops::Type*)&this->val, (typename Ops::Type)rhs); 
}

template<typename Type>
void Atomic<Type>::sub(Type rhs, AtomicOrder order)
{
    using Ops = detail::Interlocked<sizeof(Type)>;
    Ops::fetch_add((volatile typename Ops::Type*)&this->val, (typename Ops::Type)(-(typename Ops::Type)rhs)); 
}

template<typename Type>
void Atomic<Type>::mul(Type rhs, AtomicOrder order)
{
    using Ops = detail::Interlocked<sizeof(Type)>;
    typename Ops::Type* p = (volatile typename Ops::Type*)&this->val;
    typename Ops::Type old_val = Ops::load(p);
    typename Ops::Type new_val;
    do
    {
        new_val = old_val * (typename Ops::Type)rhs;
    } while (Ops::compare_exchange(p, new_val, old_val) != old_val);
}

template<typename Type>
void Atomic<Type>::div(Type rhs, AtomicOrder order)
{
    using Ops = detail::Interlocked<sizeof(Type)>;
    typename Ops::Type* p = (volatile typename Ops::Type*)&this->val;
    typename Ops::Type old_val = Ops::load(p);
    typename Ops::Type new_val;
    do
    {
        new_val = old_val / (typename Ops::Type)rhs;
    } while (Ops::compare_exchange(p, new_val, old_val) != old_val);
}

template<typename Type>
void Atomic<Type>::bit_and(Type rhs, AtomicOrder order)
{
    using Ops = detail::Interlocked<sizeof(Type)>;
    Ops::bit_and((volatile typename Ops::Type*)&this->val, (typename Ops::Type)rhs); 
}

template<typename Type>
void Atomic<Type>::bit_or(Type rhs, AtomicOrder order)
{
    using Ops = detail::Interlocked<sizeof(Type)>;
    Ops::bit_or((volatile typename Ops::Type*)&this->val, (typename Ops::Type)rhs); 
}

template<typename Type>
void Atomic<Type>::bit_xor(Type rhs, AtomicOrder order)
{
    using Ops = detail::Interlocked<sizeof(Type)>;
    Ops::bit_xor((volatile typename Ops::Type*)&this->val, (typename Ops::Type)rhs); 
}

template<typename Type>
Type Atomic<Type>::exchange(Type rhs, AtomicOrder order)
{
    using Ops = detail::Interlocked<sizeof(Type)>;
    return (Type)Ops::exchange((volatile typename Ops::Type*)&this->val, (typename Ops::Type)rhs); 
}

template<typename Type>
bool Atomic<Type>::compare_exchange_weak(Type* expected_val, Type new_val, AtomicOrder success_order, AtomicOrder fail_order)
{
    using Ops = detail::Interlocked<sizeof(Type)>;
    typename Ops::Type* p = (volatile typename Ops::Type*)&this->val;
    typename Ops::Type old = Ops::compare_exchange(p, (typename Ops::Type)new_val, (typename Ops::Type)*expected_val);
    if (old == (typename Ops::Type)*expected_val)
        return true;
    *expected_val = (Type)old;
    return false;
}

template<typename Type>
bool Atomic<Type>::compare_exchange_strong(Type* expected_val, Type new_val, AtomicOrder success_order, AtomicOrder fail_order)
{
    using Ops = detail::Interlocked<sizeof(Type)>;
    typename Ops::Type* p = (volatile typename Ops::Type*)&this->val;
    typename Ops::Type old = Ops::compare_exchange(p, (typename Ops::Type)new_val, (typename Ops::Type)*expected_val);
    if (old == (typename Ops::Type)*expected_val)
        return true;
    *expected_val = (Type)old;
    return false;
}

#else // _MSVC

template<typename Type>
Type Atomic<Type>::load(AtomicOrder order) const
{
    return __atomic_load_n(&this->val, (s32)order); 
}

template<typename Type>
void Atomic<Type>::store(Type new_val, AtomicOrder order)
{
    __atomic_store_n(&this->val, new_val, (s32)order); 
}

template<typename Type>
void Atomic<Type>::add(Type rhs, AtomicOrder order)
{
    __atomic_fetch_add(&this->val, rhs, (s32)order); 
}

template<typename Type>
void Atomic<Type>::sub(Type rhs, AtomicOrder order)
{
    __atomic_fetch_sub(&this->val, rhs, (s32)order); 
}

template<typename Type>
void Atomic<Type>::mul(Type rhs, AtomicOrder order)
{
    __atomic_fetch_mul(&this->val, rhs, (s32)order); 
}

template<typename Type>
void Atomic<Type>::div(Type rhs, AtomicOrder order)
{
    __atomic_fetch_div(&this->val, rhs, (s32)order); 
}

template<typename Type>
void Atomic<Type>::bit_and(Type rhs, AtomicOrder order)
{
    __atomic_fetch_and(&this->val, rhs, (s32)order); 
}

template<typename Type>
void Atomic<Type>::bit_or(Type rhs, AtomicOrder order)
{
    __atomic_fetch_or(&this->val, rhs, (s32)order); 
}

template<typename Type>
void Atomic<Type>::bit_xor(Type rhs, AtomicOrder order)
{
    __atomic_fetch_xor(&this->val, rhs, (s32)order); 
}

template<typename Type>
Type Atomic<Type>::exchange(Type rhs, AtomicOrder order)
{
    return __atomic_fetch_exchange_n(&this->val, rhs, (s32)order); 
}

template<typename Type>
bool Atomic<Type>::compare_exchange_weak(Type* expected_val, Type new_val, AtomicOrder success_order, AtomicOrder fail_order)
{
    return __atomic_compare_exchange_n(&this->val, expected_val, true, new_val, (s32)success_order, (s32)fail_order);
}

template<typename Type>
bool Atomic<Type>::compare_exchange_strong(Type* expected_val, Type new_val, AtomicOrder success_order, AtomicOrder fail_order)
{
    return __atomic_compare_exchange_n(&this->val, expected_val, false, new_val, (s32)success_order, (s32)fail_order);
}

#endif

} // rg

#endif // _RG_ATOMIC_HPP_
