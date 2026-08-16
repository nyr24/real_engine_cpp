#include <stdarg.h>
#include "core/basic.hpp"
#include "core/clock.hpp"
#include "core/context.hpp"
#include "collections/bits.hpp"

namespace rg
{

// Panic / unreachable.

// TODO: panic should execute have a destruction queue before exit for safety.
[[noreturn]] void panic(CString file, s32 line, CString message, ...)
{
    // Do log.
    LOG_FATAL("Panic happened at: %s %d", file, line);
    va_list args;
    va_start(args, message);
    LOG_FATAL(message, args);
    va_end(args);

    // Execute exit callbacks.
    Context* context = get_context();
    context->exit_callbacks.foreach([](const auto& cb) {
        cb();
    });

    exit(1);
}

[[noreturn]] void unreachable(CString file, s32 line, CString message, ...)
{
    // Do log.
    LOG_FATAL("Unreachable code reached at: %s %d", file, line);
    va_list args;
    va_start(args, message);
    LOG_FATAL(message, args);
    va_end(args);

    // Execute exit callbacks.
    Context* context = get_context();
    context->exit_callbacks.foreach([](const auto& cb) {
        cb();
    });

#if defined(_MSC_VER) && !defined(__clang__) // MSVC
    __assume(false);
#else // GCC, Clang
    __builtin_unreachable();
#endif
}

void assert_proc(bool expr, CString file, s32 line)
{
    if (!expr)
    {
        LOG_FATAL("Assertion failed in file: %s, line: %d", file, line);

        // Execute exit callbacks.
        Context* context = get_context();
        context->exit_callbacks.foreach([](const auto& handler) {
            handler();
        });

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

        // Execute panic handlers.
        Context* context = get_context();
        context->exit_callbacks.foreach([](const auto& handler) {
            handler();
        });
        
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

// Time.

Nanoseconds get_current_time_ns()
{
#ifdef RG_PLATFORM_WIN32
    LARGE_INTEGER ticks, frequency;
    ASSERT(::QueryPerformanceCounter(&ticks));
    ASSERT(::QueryPerformanceFrequency(&frequency));
    
    // Scale first to prevent precision loss, use 1ULL to avoid overflow
    return (ticks.QuadPart * NANO_SEC) / frequency.QuadPart;
#else
    struct timespec ts;
    s32 res = ::clock_gettime(CLOCK_MONOTONIC, &ts);
    ASSERT_EQ_ZERO(res);
    
    // Combine total seconds and the remaining nanoseconds
    return sec_to_ns((Nanoseconds)ts.tv_sec) + ts.tv_nsec;
#endif
}

// Clock.

void Clock::start()
{
    this->start_time = get_current_time_ns();
    this->progress = 0;
    this->delta = 0;
}

Nanoseconds Clock::update_and_get_delta()
{
    Nanoseconds curr_progress_from_start = get_current_time_ns() - this->start_time;
    Nanoseconds delta = curr_progress_from_start - this->progress;
	this->progress = curr_progress_from_start;
	this->delta = delta;
	return delta;
}

void Clock::update()
{
    Nanoseconds curr_progress_from_start = get_current_time_ns() - this->start_time;
    this->delta = curr_progress_from_start - this->progress;
	this->progress = curr_progress_from_start;
}

void Clock::stop()
{
    this->start_time = 0;
    this->progress = 0;
}

// Thread id.

sz get_thread_id()
{
#ifdef RG_PLATFORM_WIN32
	return (sz)::GetCurrentThreadId();
#else
	return (sz)::pthread_self();
#endif
}

// Bencher.

ScopeBencher::ScopeBencher(CString name)
{
	this->init_timestamp = get_current_time_ns();
	this->name = name;
}

ScopeBencher::~ScopeBencher()
{
	Nanoseconds scope_time_ns = get_current_time_ns() - this->init_timestamp;
	printfn("%s took %ldns %ldmis %ldms %ldsec",
		this->name, scope_time_ns, ns_to_microsec(scope_time_ns), ns_to_ms(scope_time_ns), ns_to_sec(scope_time_ns));
}

} // rg
