#include <stdarg.h>
#include "engine/entry.hpp"

namespace rg
{

intern Context context;
intern thread_local Arena* temp_alloc;
intern EngineContext engine_context;
intern VmemAllocator* persistent_allocator;

intern void context_init(Allocator* allocator);
intern void context_destroy();
intern void engine_context_init();
intern void engine_context_destroy();

void application_init()
{
    persistent_allocator = VmemAllocator::create(1 * GB);
    context_init(persistent_allocator);
    init_temp_allocator(context.allocator);
    engine_context_init();
}

void application_run()
{
    ;
}

void application_destroy()
{
    context_destroy();
    persistent_allocator->destroy();
}

// Context.

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

Context* get_context()
{
    return &context;
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

// Engine context.

void engine_context_init()
{
    engine_context.event_sys.init();
    engine_context.input_sys.init();
}

void engine_context_destroy()
{
}

EngineContext* get_engine_context()
{
    return &engine_context; 
}

// Logging (here because of dependency on 'mutex' inside Context).


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
    Context* ctx = get_context();
    ctx->logger_mutex.lock();
    printn(LOG_INTROS[level]);
    vfprintf(stdout, fmt, args);
    printn(LOG_COLOR_RESET);
    ctx->logger_mutex.unlock();
    va_end(args);
}

void log_proc_scoped(CString fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Context* ctx = get_context();
    ctx->logger_mutex.lock();
    vfprintf(stdout, fmt, args);
    ctx->logger_mutex.unlock();
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

} // rg
