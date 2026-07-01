#ifndef _RG_LOG_HPP_
#define _RG_LOG_HPP_

#include "farray.hpp"

// Logging.

namespace rg 
{

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

void log_proc(LogLevel level, CString fmt, ...);

#ifdef RG_DEBUG
    #define LOG_INFO(fmt, args) log_proc(LogLevel::INFO, fmt, args);
    #define LOG_TRACE(fmt, args) log_proc(LogLevel::TRACE, fmt, args);
    #define LOG_DEBUG(fmt, args) log_proc(LogLevel::DEBUG, fmt, args);
    #define LOG_TEST(fmt, args) log_proc(LogLevel::TEST, fmt, args);
    #define LOG_WARN(fmt, args) log_proc(LogLevel::WARN, fmt, args);
    #define LOG_ERROR(fmt, args) log_proc(LogLevel::ERROR, fmt, args);
    #define LOG_FATAL(fmt, args) log_proc(LogLevel::FATAL, fmt, args);
#else
    #define LOG_INFO(fmt, args);
    #define LOG_TRACE(fmt, args);
    #define LOG_DEBUG(fmt, args);
    #define LOG_TEST(fmt, args);
    #define LOG_WARN(fmt, args);
    #define LOG_ERROR(fmt, args) log_proc(LogLevel::ERROR, fmt, args);
    #define LOG_FATAL(fmt, args) log_proc(LogLevel::FATAL, fmt, args);
#endif

} // rg

#endif // _RG_LOG_HPP_
