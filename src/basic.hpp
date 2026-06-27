#ifndef _RG_BASIC_HPP_
#define _RG_BASIC_HPP_

#include "cstddef"
#include "cstdint"
#include "cstdlib"
#include "cassert"
#include "cstdio"
#include "cstring"

// Types and keywords.

typedef uintptr_t uptr;
typedef ptrdiff_t sz;
typedef size_t usz;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float f32;
typedef double f64;

#define internal static
#define cast static_cast
#define bitcast reinterpret_cast
#define null nullptr

#define ASSERT(expr) assert((expr))
#define ASSERT_MSG(expr, msg) assert((expr) && (msg))
#define ASSERT_FALSE(expr) assert((!expr))
#define ASSERT_FALSE_MSG(expr, msg) assert((!expr) && (msg))
#define ASSERT_NON_NULL(expr, msg) assert(((expr) != nullptr) && (msg))
#define TODO(msg) assert(false && (msg))

#define printn(msg) fputs(msg, stdout)
#define printfn(fmt, args...) fprintf(stdout, fmt "\n", args)
#define eprintn(msg) fputs(msg, stderr)
#define eprintfn(fmt, args...) fprintf(stderr, fmt "\n", args)

namespace rg
{

const sz DEFAULT_MEM_ALIGNMENT = sizeof(uptr) * 2;

template<typename Type>
constexpr Type max(Type a, Type b)
{
	return a > b ? a : b;
}

template<typename Type>
constexpr Type min(Type a, Type b)
{
	return a < b ? a : b;
}

template<typename Type>
constexpr Type clamp(Type val, Type lower, Type upper)
{
	return max(lower, min(val, upper));
}

template<typename Type>
inline Type swap(Type* a, Type* b)
{
	Type temp = *a;
	*a = *b;
	*b = temp;
}

template<typename Type>
constexpr bool is_power_of_two(Type val)
{
	return val != 0 && (val & (val - 1)) == 0;
}

template<typename Type>
constexpr Type next_power_of_2(Type x)
{
	if (x <= 1) return Type(1);
	if (x == 2) return Type(2);

	Type v = x - Type(1);

	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;

	if constexpr (sizeof(x) >= 2)  v |= v >> 8;
	if constexpr (sizeof(x) >= 4)  v |= v >> 16;
	if constexpr (sizeof(x) >= 8)  v |= v >> 32;
	if constexpr (sizeof(x) >= 16) v |= v >> 64;

	return v + Type(1);
}

template<typename Type>
constexpr Type align(Type val, sz alignment)
{
	ASSERT(is_power_of_two(alignment));
	return (val + alignment - 1) & ~(alignment - 1);
}

template<typename PtrType>
constexpr PtrType align_ptr(PtrType ptr, sz alignment)
{
	ASSERT(is_power_of_two(alignment));
	return PtrType(align((uptr)ptr, alignment));
}

inline void mem_copy(void* dest, void* src, sz len)
{
	memcpy(dest, src, len);
}

inline bool mem_compare(void* a, void* b, sz len)
{
	return memcmp(a, b, len) == 0;
}

template<typename Type>
inline void mem_set(void* ptr, sz len, Type val)
{
	memset(ptr, val, len);
}

inline void mem_zero(void* ptr, sz len)
{
	memset(ptr, 0, len);
}

constexpr sz alignment_for_allocation(sz alignment)
{
	return max(alignment, DEFAULT_MEM_ALIGNMENT);
}

// Defer.

template <typename Func>
struct _Defer
{
	Func f;
	_Defer(Func f) : f(f) {}
	~_Defer() { f(); }
};

template <typename Func>
_Defer<Func> defer_func(Func f) {
	return _Defer<Func>(f);
}

#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define DEFER_3(x)    DEFER_2(x, __COUNTER__)
#define defer(code)   auto DEFER_3(_defer_) = defer_func([&](){code;})

// Allocator interface.

struct AllocatorVtable;

struct Allocator
{
    AllocatorVtable* vtable;
};

struct AllocatorVtable
{
	void* (*allocate)     (Allocator* alloc, sz size, sz alignment, bool zero_mem);
	void* (*reallocate)   (Allocator* alloc, void* ptr, sz new_size, sz alignment);
	void  (*free)         (Allocator* alloc, void* ptr);
	void  (*display_info) (Allocator* alloc);
};

// For convenient access.
void* allocator_allocate(Allocator* alloc, sz size, sz alignment = 0, bool zero_mem = false);
void* allocator_reallocate(Allocator* alloc, void* ptr, sz new_size, sz alignment = 0);
void  allocator_free(Allocator* alloc, void* ptr);
void  allocator_display_info(Allocator* alloc);

} // rg

#endif // _RG_BASIC_HPP_
