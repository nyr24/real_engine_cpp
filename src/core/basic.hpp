#ifndef _RG_BASIC_HPP_
#define _RG_BASIC_HPP_

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

#ifdef _WIN32
	#define RG_PLATFORM_WIN32
	#include <windows.h>
	#define FMT_PLACEHOLDER_NULL "%S"
	#define FMT_PLACEHOLDER_LEN "%.*S"
#else
	#define RG_PLATFORM_POSIX
	#include <fcntl.h>
	#include <unistd.h>
	#include <sys/stat.h>
	#include <sys/mman.h>	
	#define FMT_PLACEHOLDER_NULL "%s"
	#define FMT_PLACEHOLDER_LEN "%.*s"
#endif // _WIN32

#ifdef PAGE_SIZE
#define RG_PAGE_SIZE PAGE_SIZE; 
#else
#define RG_PAGE_SIZE 4096; 
#endif

#ifdef THREAD_COUNT
#define RG_THREAD_COUNT THREAD_COUNT; 
#else
#define RG_THREAD_COUNT 4096; 
#endif

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
#define persist static
#define cast static_cast
#define bitcast reinterpret_cast
#define null nullptr
#define alias using

// Asserts.

#if defined(RG_DEBUG)
    #if defined(_MSC_VER)
        extern void __cdecl __debugbreak(void);
        #define DEBUG_BREAK() __debugbreak()
    #elif ( (!defined(__NACL__)) && ((defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))) )
        #define DEBUG_BREAK() __asm__ __volatile__ ( "int $3\n\t" )
    #elif (defined(__GNUC__) || defined(__clang__)) && defined(__riscv)
        #define DEBUG_BREAK() __asm__ __volatile__ ( "ebreak\n\t" )
    #elif ( defined(__APPLE__) && (defined(__arm64__) || defined(__aarch64__)) )
        #define DEBUG_BREAK() __asm__ __volatile__ ( "brk #22\n\t" )
    #elif defined(__APPLE__) && defined(__arm__)
        #define DEBUG_BREAK() __asm__ __volatile__ ( "bkpt #22\n\t" )
    #elif defined(__386__) && defined(__WATCOMC__)
        #define DEBUG_BREAK() { _asm { int 0x03 } }
    #elif defined(HAVE_SIGNAL_H) && !defined(__WATCOMC__)
        #define DEBUG_BREAK() raise(SIGTRAP)
    #else
        #define DEBUG_BREAK()
    #endif
#else
    #define DEBUG_BREAK()
#endif //_DEBUG

namespace rg
{

void assert_proc(bool expr);
void assert_msg_proc(bool expr, CString fmt, ...);

#ifdef RG_DEBUG
	#define ASSERT(expr) assert_proc((expr))
	#define ASSERT_MSG(expr, msg, ...) assert_msg_proc((expr), (msg), ##__VA_ARGS__)
	#define ASSERT_EQ_ZERO(expr) ASSERT_MSG((expr) == 0, "Must be equal to 0")
	#define ASSERT_NON_ZERO(expr) ASSERT_MSG((expr) != 0, "Mustn't be equal to 0")
	#define ASSERT_GREATER_ZERO(expr) ASSERT_MSG((expr) > 0, "Must be greater than 0")
	#define ASSERT_GREATER_EQ_ZERO(expr) ASSERT_MSG((expr) >= 0, "Must be greater or equal to 0")
	#define ASSERT_BELOW_ZERO(expr) ASSERT_MSG((expr) < 0, "Must be smaller than 0")
	#define ASSERT_BELOW_EQ_ZERO(expr) ASSERT_MSG((expr) <= 0, "Must be smaller or equal to 0")
	#define ASSERT_IN_BOUNDS(expr) ASSERT_MSG((expr), "Must be in bounds")
	#define ASSERT_NON_NULL(expr) ASSERT_MSG((expr) != null, "Mustn't be null")
	#define ASSERT_NON_EMPTY(expr) ASSERT_MSG(!(expr)->is_empty(), "Must be non-empty")
	#define ASSERT_NON_EMPTY_VAL(expr) ASSERT_MSG(!(expr).is_empty(), "Must be non-empty")
	#define ASSERT_INITIALIZED(expr) ASSERT_MSG(expr->is_initialized(), "Must be initialized")
	#define ASSERT_INITIALIZED_VAL(expr) ASSERT_MSG(expr.is_initialized(), "Must be initialized")
	#define ASSERT_NON_INITIALIZED(expr) ASSERT_MSG(!expr->is_initialized(), "Not expected to be initialized")
	#define ASSERT_NON_INITIALIZED_VAL(expr) ASSERT_MSG(!expr.is_initialized(), "Not expected to be initialized")
	#define TODO(msg) ASSERT_MSG(false, (msg))
#else
	#define ASSERT(expr)
	#define ASSERT_MSG(expr, msg, ...)
	#define ASSERT_EQ_ZERO(expr)
	#define ASSERT_NON_ZERO(expr)
	#define ASSERT_GREATER_ZERO(expr)
	#define ASSERT_GREATER_EQ_ZERO(expr)
	#define ASSERT_BELOW_ZERO(expr)
	#define ASSERT_BELOW_EQ_ZERO(expr)
	#define ASSERT_IN_BOUNDS(expr)
	#define ASSERT_NON_NULL(expr)
	#define ASSERT_NON_EMPTY(expr)
	#define ASSERT_NON_EMPTY_VAL(expr)
	#define ASSERT_INITIALIZED(expr)
	#define ASSERT_INITIALIZED_VAL(expr)
	#define ASSERT_NON_INITIALIZED(expr)
	#define ASSERT_NON_INITIALIZED_VAL(expr)
	#define TODO(msg)
#endif

#define CONCAT(a, b) a b

// Statically determine cstring length without doing costly strlen().
#define CSTR_SIZED(cstr) cstr, sizeof(cstr) - 1
#define CSTR_SIZED_NULL(cstr) cstr, sizeof(cstr)
#define CARRAY_LEN(carr) sizeof(carr) / sizeof(carr[0])
#define CARRAY_SIZED(carr) carr, CARRAY_LEN(carr)
#define printn(msg) fputs(msg, stdout)
#define printfn(fmt, ...) fprintf(stdout, fmt "\n", ##__VA_ARGS__)
#define eprintn(msg) fputs(msg, stderr)
#define eprintfn(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)

#ifdef RG_PLATFORM_WIN32
constexpr FileHandle FILE_HANDLE_INVALID = INVALID_HANDLE_VALUE;
#else
constexpr FileHandle FILE_HANDLE_INVALID = -1;
#endif
constexpr sz INDEX_INVALID = -1;
constexpr sz KB = 1024;
constexpr sz MB = 1024 * 1024;
constexpr sz GB = 1024 * 1024 * 1024;
constexpr sz DEFAULT_MEM_ALIGNMENT = sizeof(uptr) * 2;

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
void log_proc_scoped(CString fmt, ...);
void start_log_scope(LogLevel level, CString fmt);
void start_log_scope(LogLevel level);
void end_log_scope();

#ifdef RG_DEBUG
    #define LOG_INFO(fmt, ...) log_proc(LogLevel::INFO, fmt, ##__VA_ARGS__)
    #define LOG_TRACE(fmt, ...) log_proc(LogLevel::TRACE, fmt, ##__VA_ARGS__)
    #define LOG_DEBUG(fmt, ...) log_proc(LogLevel::DEBUG, fmt, ##__VA_ARGS__)
    #define LOG_TEST(fmt, ...) log_proc(LogLevel::TEST, fmt, ##__VA_ARGS__)
    #define LOG_WARN(fmt, ...) log_proc(LogLevel::WARN, fmt, ##__VA_ARGS__)
    #define LOG_ERROR(fmt, ...) log_proc(LogLevel::ERROR, fmt, ##__VA_ARGS__)
    #define LOG_FATAL(fmt, ...) log_proc(LogLevel::FATAL, fmt, ##__VA_ARGS__)
    #define LOG_SCOPED(fmt, ...) log_proc_scoped(fmt, ##__VA_ARGS__)
    #define LOG_SCOPED_PRESERVE_RELEASE(fmt, ...) log_proc_scoped(fmt, ##__VA_ARGS__)
#else
    #define LOG_INFO(fmt, ...)
    #define LOG_TRACE(fmt, ...)
    #define LOG_DEBUG(fmt, ...)
    #define LOG_TEST(fmt, ...)
    #define LOG_WARN(fmt, ...)
    #define LOG_ERROR(fmt, ...) log_proc(LogLevel::ERROR, fmt, ##__VA_ARGS__)
    #define LOG_FATAL(fmt, ...) log_proc(LogLevel::FATAL, fmt, ##__VA_ARGS__)
    #define LOG_SCOPED(fmt, ...)
    #define LOG_SCOPED_PRESERVE_RELEASE(fmt, ...) log_proc_scoped(fmt, ##__VA_ARGS__)
#endif

struct ScopedLogger
{
	ScopedLogger(LogLevel level, CString msg) { start_log_scope(level, msg); }
	ScopedLogger(LogLevel level) { start_log_scope(level); }
	~ScopedLogger() { end_log_scope(); }
};

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

#define ASSERT_POW_OF_TWO(expr) ASSERT_MSG(is_power_of_two(expr), "Must be a power of 2")
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
	ASSERT_POW_OF_TWO(alignment);
	return (val + alignment - 1) & ~(alignment - 1);
}

template<typename Type>
constexpr Type align_backward(Type val, sz alignment)
{
	ASSERT_POW_OF_TWO(alignment);
	return val - (val & alignment - 1);
}

template<typename PtrType>
constexpr PtrType align_ptr(PtrType ptr, sz alignment = 0)
{
	alignment = alignment ? alignment : alignof(PtrType);
	ASSERT_POW_OF_TWO(alignment);
	return PtrType(align((uptr)ptr, alignment));
}

template<typename PtrType>
constexpr PtrType align_ptr_backward(PtrType ptr, sz alignment = 0)
{
	alignment = alignment ? alignment : alignof(PtrType);
	ASSERT_POW_OF_TWO(alignment);
	return (uptr)ptr - ((uptr)ptr & alignment - 1);
}

template<typename PtrType>
constexpr bool is_ptr_aligned(PtrType ptr, sz alignment)
{
	ASSERT_POW_OF_TWO(alignment);
	return (uptr(ptr) & alignment - 1) == 0;
}

#define ASSERT_ALIGNED(ptr, align) ASSERT_MSG(is_ptr_aligned(ptr, align), "Pointer must be aligned")

inline void _mem_copy(void* dest, void* src, sz byte_size)
{
	memcpy(dest, src, byte_size);
}


#define mem_copy(a, b, size) _mem_copy((void*)a, (void*)b, size)

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
_Defer<Func> defer_func(Func f)
{
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
    const AllocatorVtable* vtable;
};

struct AllocatorVtable
{
	void* (*allocate)     (Allocator* alloc, sz size, sz alignment, bool zero_mem);
	void* (*reallocate)   (Allocator* alloc, void* ptr, sz new_size, sz alignment);
	bool  (*resize)       (Allocator* alloc, void* ptr, sz new_size, sz alignment);
	void  (*free)         (Allocator* alloc, void* ptr);
	void  (*display_info) (Allocator* alloc);
};

// For convenient access.
void* allocator_allocate(Allocator* alloc, sz size, sz alignment = 0, bool zero_mem = false);
void* allocator_reallocate(Allocator* alloc, void* ptr, sz new_size, sz alignment = 0);
bool  allocator_resize(Allocator* alloc, void* ptr, sz new_size, sz alignment = 0);
void  allocator_free(Allocator* alloc, void* ptr);
void  allocator_display_info(Allocator* alloc);

// Maybe.

template<typename Type>
struct Maybe
{
    Type val;
    bool is_ok;

    Maybe(): is_ok{false} {}
    Maybe(bool want_ok): is_ok{want_ok} {}
    Maybe(Type new_val, bool want_ok): val{new_val}, is_ok{want_ok} {}

	void set_val(Type val);
	void set_empty();
    inline operator bool() { return is_ok; }
};

template<typename Type>
inline void Maybe<Type>::set_val(Type val)
{
	this->is_ok = true;
	this->val = val;
}

template<typename Type>
inline void Maybe<Type>::set_empty()
{
	this->is_ok = false;
}

// Tuples.

template<typename First, typename Second>
struct Pair
{
	First first;
	Second second;
};

template<typename First, typename Second, typename Third>
struct Triplet
{
	First first;
	Second second;
	Third third;
};

template<typename First, typename Second, typename Third, typename Fourth>
struct Quadriplet
{
	First first;
	Second second;
	Third third;
	Fourth fourth;
};

// Rng

struct XorshiftRng
{
    u64 state;

    XorshiftRng(u64 seed)
	{
	    if (seed == 0)
	    {
	        state = u64(time(null));
	    }
	    else
	    {
	        state = seed;
	    }
	};

    template<typename Type>
    Type next();
    template<typename Type>
    Type next_in_range(Type min, Type max);
};

// Hash (fnv1a).

constexpr u64 FNV_PRIME = 1099511628211ull;
constexpr u64 FNV_OFFSET_BASIS = 14695981039346656037ull;

u64 hash_fnv(char* bytes, sz count);
bool is_space(char c);

// Move

template<typename Type>
struct RemoveReference
{
	using type = Type;
};

template<typename Type>
struct RemoveReference<Type&>
{
	using type = Type;
};

template<typename Type>
struct RemoveReference<Type&&>
{
	using type = Type;
};

template<typename Type>
[[__nodiscard__,__gnu__::__always_inline__]]
auto move(Type&& type) noexcept
{
	return static_cast<typename RemoveReference<Type>::type&&>(type);
}

} // rg

#endif // _RG_BASIC_HPP_
