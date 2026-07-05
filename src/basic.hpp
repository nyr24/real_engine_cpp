#ifndef _RG_BASIC_HPP_
#define _RG_BASIC_HPP_

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <cstring>

#ifdef _WIN32
	#define RG_PLATFORM_WIN32
	#include <windows.h>
	#define FMT_STR_NULL "%S"
	#define FMT_STR_LEN "%.*S"
#else
	#define RG_PLATFORM_POSIX
	#include <fcntl.h>
	#include <unistd.h>
	#include <sys/stat.h>
	#include <sys/mman.h>	
	#define FMT_STR_NULL "%s"
	#define FMT_STR_LEN "%.*s"
#endif // _WIN32

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
typedef const char* CString;
typedef s32 FileHandle;
typedef time_t FileTimeUnit;

#define intern static
#define cast static_cast
#define bitcast reinterpret_cast
#define null nullptr
#define alias using

#define ASSERT(expr) assert((expr))
#define ASSERT_MSG(expr, msg) assert((expr) && (msg))
#define ASSERT_EQ_ZERO(expr) assert((expr) == 0 && "Must be equal to 0")
#define ASSERT_NON_ZERO(expr) assert((expr) != 0 && "Mustn't be equal to 0")
#define ASSERT_GREATER_ZERO(expr) assert((expr) > 0 && "Must be greater than 0")
#define ASSERT_GREATER_EQ_ZERO(expr) assert((expr) >= 0 && "Must be greater or equal to 0")
#define ASSERT_BELOW_ZERO(expr) assert((expr) < 0 && "Must be smaller than 0")
#define ASSERT_BELOW_EQ_ZERO(expr) assert((expr) <= 0 && "Must be smaller or equal to 0")
#define ASSERT_IN_BOUNDS(expr) assert((expr) && "Must be in bounds")
#define ASSERT_NON_NULL(expr) assert((expr) != null && "Mustn't be null")
#define ASSERT_NON_EMPTY(expr) assert(!(expr)->is_empty() && "Must be non-empty");
#define ASSERT_NON_EMPTY_VAL(expr) assert(!(expr).is_empty() && "Must be non-empty");
#define ASSERT_INITIALIZED(expr) assert(expr->is_initialized() && "Must be initialized");
#define ASSERT_INITIALIZED_VAL(expr) assert(expr.is_initialized() && "Must be initialized");
#define ASSERT_NON_INITIALIZED(expr) assert(!expr->is_initialized() && "Not expected to be initialized");
#define ASSERT_NON_INITIALIZED_VAL(expr) assert(!expr.is_initialized() && "Not expected to be initialized");
#define ASSERT_STATIC(expr) static_assert((expr))
#define ASSERT_STATIC_MSG(expr, msg) static_assert((expr), (msg))
#define TODO(msg) assert(false && (msg))
#define CONCAT(a, b) a b

#define GET_FMT_STR(str_p) str_p->count, str_p->data
#define GET_FMT_STR_VAL(str) str.count, str.data
#define GET_FMT_STR_VIEW(str_view) str_view.count, str_view.ptr
// Statically determine cstring length without doing costly strlen().
#define CSTR_SIZED(cstr) cstr, sizeof(cstr) - 1
#define CSTR_SIZED_NULL(cstr) cstr, sizeof(cstr)
#define CARRAY_LEN(carr) sizeof(carr) / sizeof(carr[0])
#define CARRAY_SIZED(carr) carr, CARRAY_LEN(carr)
#define printn(msg) fputs(msg, stdout)
#define printfn(fmt, args...) fprintf(stdout, fmt "\n", args)
#define eprintn(msg) fputs(msg, stderr)
#define eprintfn(fmt, args...) fprintf(stderr, fmt "\n", args)

namespace rg
{

#ifdef RG_PLATFORM_WIN32
constexpr FileHandle FILE_HANDLE_INVALID = INVALID_HANDLE_VALUE;
#else
constexpr FileHandle FILE_HANDLE_INVALID = -1;
#endif
constexpr sz INDEX_INVALID = -1;
constexpr sz DEFAULT_MEM_ALIGNMENT = sizeof(uptr) * 2;
constexpr sz KB = 1024;
constexpr sz MB = 1024 * 1024;
constexpr sz GB = 1024 * 1024 * 1024;

struct SystemInfo
{
	sz thread_count;
	sz page_size;
};

void get_system_info(SystemInfo* sys_info);

// Logging.

enum struct LogLevel
{
	INFO,
	TRACE,
	DEBUG,
	TEST,
	WARN,
	ERROR,
	FATAL,
	EnumSize
};

void log_proc(LogLevel level, CString fmt, ...);

#ifdef RG_DEBUG
    #define LOG_INFO(fmt, ...) log_proc(LogLevel::INFO, fmt, ##__VA_ARGS__);
    #define LOG_TRACE(fmt, ...) log_proc(LogLevel::TRACE, fmt, ##__VA_ARGS__);
    #define LOG_DEBUG(fmt, ...) log_proc(LogLevel::DEBUG, fmt, ##__VA_ARGS__);
    #define LOG_TEST(fmt, ...) log_proc(LogLevel::TEST, fmt, ##__VA_ARGS__);
    #define LOG_WARN(fmt, ...) log_proc(LogLevel::WARN, fmt, ##__VA_ARGS__);
    #define LOG_ERROR(fmt, ...) log_proc(LogLevel::ERROR, fmt, ##__VA_ARGS__);
    #define LOG_FATAL(fmt, ...) log_proc(LogLevel::FATAL, fmt, ##__VA_ARGS__);
#else
    #define LOG_INFO(fmt, ...);
    #define LOG_TRACE(fmt, ...);
    #define LOG_DEBUG(fmt, ...);
    #define LOG_TEST(fmt, ...);
    #define LOG_WARN(fmt, ...);
    #define LOG_ERROR(fmt, ...) log_proc(LogLevel::ERROR, fmt, ##__VA_ARGS__);
    #define LOG_FATAL(fmt, ...) log_proc(LogLevel::FATAL, fmt, ##__VA_ARGS__);
#endif


template<typename Type>
constexpr Type max(Type a, Type b)
{
	return a > b ? a : b;
}

template<typename Type>
constexpr Type max(Type a, Type b, Type c)
{
	Type res = a;
	res = max(res, b);
	res = max(res, c);
	return res;
}

template<typename Type>
constexpr Type max(Type a, Type b, Type c, Type d)
{
	Type res = a;
	res = max(res, b);
	res = max(res, c);
	res = max(res, d);
	return res;
}

template<typename Type>
constexpr Type min(Type a, Type b)
{
	return a < b ? a : b;
}

template<typename Type>
constexpr Type min(Type a, Type b, Type c)
{
	Type res = a;
	res = min(res, b);
	res = min(res, c);
	return res;
}

template<typename Type>
constexpr Type min(Type a, Type b, Type c, Type d)
{
	Type res = a;
	res = min(res, b);
	res = min(res, c);
	res = min(res, d);
	return res;
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

#define ASSERT_POW_OF_TWO(expr) assert(is_power_of_two(expr) && "Must be a power of 2")
#define ASSERT_POW_OF_TWO_STATIC(expr) static_assert(is_power_of_two(expr), "Must be a power of 2")

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

inline void mem_copy(void* dest, void* src, sz byte_size)
{
	memcpy(dest, src, byte_size);
}

inline bool mem_compare(void* a, void* b, sz byte_size)
{
	return memcmp(a, b, byte_size) == 0;
}

template<typename Type>
inline void mem_set(void* ptr, sz byte_size, Type val)
{
	memset(ptr, val, byte_size);
}

inline void mem_zero(void* ptr, sz byte_size)
{
	memset(ptr, 0, byte_size);
}

constexpr sz alignment_for_allocation(sz alignment)
{
	return max(alignment, DEFAULT_MEM_ALIGNMENT);
}

[[noreturn]] void panic(CString message = "", ...);
[[noreturn]] void unreachable(CString message = "", ...);

// Defer.

template<typename Func>
struct _Defer
{
	Func f;
	_Defer(Func f) : f(f) {}
	~_Defer() { f(); }
};

template<typename Func>
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

// Maybe.

template<typename Type>
struct Maybe
{
    Type val;
    bool has_value;

	void set_val(Type val);
	void set_empty();
    inline operator bool() { return has_value; }
};

template<typename Type>
inline Maybe<Type> maybe_create(Type val)
{
    return Maybe{ val, true };
}

template<typename Type>
inline Maybe<Type> maybe_empty()
{
    return Maybe<Type>{ .has_value = false };
}

template<typename Type>
inline void Maybe<Type>::set_val(Type val)
{
	this->has_value = true;
	this->val = val;
}

template<typename Type>
inline void Maybe<Type>::set_empty()
{
	this->has_value = false;
}

// Hash (fnv1a).

constexpr u64 FNV_PRIME = 1099511628211ull;
constexpr u64 FNV_OFFSET_BASIS = 14695981039346656037ull;

} // rg

#endif // _RG_BASIC_HPP_
