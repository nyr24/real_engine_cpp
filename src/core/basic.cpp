#include <cstdarg>
#include "core/basic.hpp"
#include "collections/farray.hpp"

namespace rg
{

[[noreturn]] void panic(CString message, ...)
{
    LOG_FATAL("{}", message);
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
    "\x1b[1;28m[DEBUG]: ",
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
    fputs(LOG_INTROS[level], stdout);
    vfprintf(stdout, fmt, args);
    fputs(LOG_COLOR_RESET, stdout);
    va_end(args);
}

} // rg
