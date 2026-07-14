#include <stdarg.h>
#include <pthread.h>
#include "core/basic.hpp"
#include "collections/farray.hpp"
#include "core/bits.hpp"
#include "core/context.hpp"

namespace rg
{

// AppContext.

void AppContext::init()
{
    this->logger_mutex.init();
}

void AppContext::destroy()
{
    this->logger_mutex.destroy();
}

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
    auto& app_ctx = AppContext::get();
    app_ctx.logger_mutex.lock();
    printn(LOG_INTROS[level]);
    vfprintf(stdout, fmt, args);
    printn(LOG_COLOR_RESET);
    app_ctx.logger_mutex.unlock();
    va_end(args);
}

void log_proc_scoped(CString fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    auto& app_ctx = AppContext::get();
    app_ctx.logger_mutex.lock();
    vfprintf(stdout, fmt, args);
    app_ctx.logger_mutex.unlock();
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
