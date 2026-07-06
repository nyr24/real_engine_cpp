#include <cstdarg>
#include <unistd.h>
#include "basic.hpp"
#include "system.hpp"

namespace rg
{

void get_system_info(Allocator* alloc, SystemInfo* sys_info)
{
#ifdef RG_PLATFORM_WIN32
    SYSTEM_INFO win_sys_info;
    GetSystemInfo(&win_sys_info);
    sys_info->thread_count = sz(win_sys_info.dwNumberOfProcessors);
    sys_info->page_size = sz(win_sys_info.dwPageSize);
#else
    sys_info->thread_count = sz(::sysconf(_SC_NPROCESSORS_CONF));
    sys_info->page_size = sz(::sysconf(_SC_PAGESIZE));
    ASSERT_MSG(sys_info->thread_count != -1 && sys_info->page_size != -1, "Error while getting system parameters");
#endif
    sys_info->cwd = get_cwd(alloc);
}

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
