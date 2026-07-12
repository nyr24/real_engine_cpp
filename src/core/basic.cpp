#include <stdarg.h>
#include <pthread.h>
#include "core/basic.hpp"
#include "collections/farray.hpp"

namespace rg
{

[[noreturn]] void panic(CString message, ...)
{
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
    printn(LOG_INTROS[level]);
    vfprintf(stdout, fmt, args);
    printn(LOG_COLOR_RESET);
    va_end(args);
}

void log_proc_scoped(CString fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
}

void start_log_scope(LogLevel level, CString msg)
{
    printfn("%s %s", LOG_INTROS[level], msg);
}

void start_log_scope(LogLevel level)
{
    printn(LOG_INTROS[level]);
}

void end_log_scope()
{
    printn(LOG_COLOR_RESET);
}

void assert_proc(bool expr)
{
    if (!expr)
    {
        DEBUG_BREAK();
        exit(1);
    }
}

void assert_msg_proc(bool expr, CString fmt, ...)
{
    if (!expr)
    {
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

} // rg
