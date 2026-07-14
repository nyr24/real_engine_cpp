#include <stdarg.h>
#include "core/basic.hpp"
#include "collections/farray.hpp"
#include "core/bits.hpp"
#include "core/context.hpp"

namespace rg
{

// Context.

intern Context context;
thread_local Arena* temp_alloc;

void context_init(Allocator* allocator)
{
    context.allocator = allocator;
    context.rng.init();
    context.logger_mutex.init();
}

void context_destroy()
{
    context.logger_mutex.destroy();
}

Context& get_context()
{
    return context;
}

void init_temp_allocator(Allocator* backing_alloc, sz capacity)
{
    temp_alloc = Arena::create(backing_alloc, capacity);
}

Arena* get_temp_allocator()
{
    ASSERT_NON_NULL(temp_alloc);
    return temp_alloc;
}

// Xorshift rng.

void XorshiftRng::init(u64 seed)
{
    if (seed == 0)
    {
        state = u64(::time(null));
    }
    else
    {
        state = seed;
    }
};

u64 XorshiftRng::next_int()
{
    u64 x = this->state;
    x ^= x << 12;
    x ^= x >> 25;
    x ^= x << 27;
    this->state = x;
    return x;
}

f32 XorshiftRng::next_float()
{
    return (f32)next_int() * 0x1.0p-32f;
}

u64 XorshiftRng::next_int_in_range(u64 min, u64 max)
{
    if (min >= max) return min;
    u64 range = max - min + 1;
    return min + (this->next_int() % range);
}

f32 XorshiftRng::next_float_in_range(f32 min, f32 max)
{
    if (min >= max) return min;
    return min + (this->next_float() * (max - min));
}

// Panic / unreachable.

// TODO: panic should execute have a destruction queue before exit for safety.
[[noreturn]] void panic(CString message, ...)
{
    LOG_FATAL("Panic happened at: %s %d", __FILE__, __LINE__);
    va_list args;
    va_start(args, message);
    LOG_FATAL(message, args);
    va_end(args);
    exit(1);
}

[[noreturn]] void unreachable(CString message, ...)
{
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
    __assume(false);
#else // GCC, Clang
    __builtin_unreachable();
#endif
}

// Logging

constexpr EnumArray<CString, LogLevel> LOG_COLORS = {
    "\x1b[1;32m",
    "\x1b[1;34m",
    "\x1b[1;34m",
    "\x1b[45;37m",
    "\x1b[1;33m",
    "\x1b[1;31m",
    "\x1b[0;41m",
};

constexpr EnumArray<CString, LogLevel> LOG_INTROS = {
    "\x1b[1;32m[INFO]: ",
    "\x1b[1;34m[TRACE]: ",
    "\x1b[1;34m[DEBUG]: ",
    "\x1b[45;37m[TEST]: ",
    "\x1b[1;33m[WARN]: ",
    "\x1b[1;31m[ERROR]: ",
    "\x1b[0;41m[FATAL]: ",
};

const CString LOG_COLOR_RESET = "\x1b[0m\n";

void log_proc(LogLevel level, CString fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    auto& context = get_context();
    context.logger_mutex.lock();
    printn(LOG_INTROS[level]);
    vfprintf(stdout, fmt, args);
    printn(LOG_COLOR_RESET);
    context.logger_mutex.unlock();
    va_end(args);
}

void log_proc_scoped(CString fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    auto& context = get_context();
    context.logger_mutex.lock();
    vfprintf(stdout, fmt, args);
    context.logger_mutex.unlock();
    va_end(args);
}

void set_log_scope(LogLevel level, CString msg)
{
    printfn("%s %s", LOG_INTROS[level], msg);
}

void set_log_scope(LogLevel level)
{
    printn(LOG_COLORS[level]);
}

void reset_log_scope()
{
    printn(LOG_COLOR_RESET);
}

void assert_proc(bool expr, CString file, s32 line)
{
    if (!expr)
    {
        LOG_FATAL("Assertion failed in file: %s, line: %d", file, line);
        DEBUG_BREAK();
        exit(1);
    }
}

void assert_msg_proc(bool expr, CString file, s32 line, CString fmt, ...)
{
    if (!expr)
    {
        LOG_FATAL("Assertion failed in file: %s, line: %d", file, line);
        va_list args;
        va_start(args, fmt);
        LOG_FATAL(fmt, args);
        va_end(args);
        DEBUG_BREAK();
        exit(1);
    }
}

u64 hash_fnv(char* start, sz count)
{
    u64 hash = FNV_OFFSET_BASIS;
    char* curr = start;
    char* end = start + count;
    for (; curr != end; ++curr)
    {
        hash ^= *curr;
        hash *= FNV_PRIME;
    }
    return hash;
}

// Bits.

template<>
sz bit_popcount(u64 storage)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcountll(storage);
#elif defined(_MSC_VER)
    return __popcnt64(storage);
#else
    // Fallback if no compiler support
    return software_popcount(storage); 
#endif
}

template<>
sz bit_popcount(u32 storage)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcount(storage);
#elif defined(_MSC_VER)
    return __popcnt(storage);
#else
    // Fallback if no compiler support
    return software_popcount(storage); 
#endif
}

template<>
sz bit_popcount(u16 storage)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcount(storage);
#elif defined(_MSC_VER)
    return __popcnt16(storage);
#else
    // Fallback if no compiler support
    return software_popcount(storage); 
#endif
}

template<>
sz bit_popcount(u8 storage)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcount(storage);
#elif defined(_MSC_VER)
    return __popcnt16((u16)storage);
#else
    // Fallback if no compiler support
    return software_popcount(storage); 
#endif
}

} // rg
