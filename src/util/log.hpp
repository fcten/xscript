#ifndef _XSCRIPT_UTIL_LOG_HPP_
#define _XSCRIPT_UTIL_LOG_HPP_

#include <iostream>

#define COLOR_RESET   "\033[0m"
#define COLOR_BLACK   "\033[30m"
#define COLOR_GRAY    "\033[90m"
#define COLOR_RED     "\033[91m"
#define COLOR_GREAN   "\033[92m"
#define COLOR_YELLOW  "\033[93m"
#define COLOR_BLUE    "\033[94m"
#define COLOR_MAGENTA "\033[95m"
#define COLOR_CYAN    "\033[96m"
#define COLOR_WHITE   "\033[97m"

#define XLOG_PRINT(level) std::cout << "[" << level << "] [" \
    << COLOR_WHITE << __FILENAME__ << ":" \
    << __LINE__ << COLOR_RESET << "] "

#define XLOG_FATAL XLOG_PRINT(COLOR_RED "FATAL" COLOR_RESET)
#define XLOG_ERROR XLOG_PRINT(COLOR_RED "ERROR" COLOR_RESET)
#define XLOG_WARN  XLOG_PRINT(COLOR_YELLOW "WARN" COLOR_RESET)
#define XLOG_INFO  XLOG_PRINT(COLOR_GRAY "INFO" COLOR_RESET)
#define XLOG_DEBUG XLOG_PRINT(COLOR_WHITE "DEBUG" COLOR_RESET)
#define XLOG_TRACE XLOG_PRINT(COLOR_WHITE "TRACE" COLOR_RESET)

#define XLOG(level) XLOG_ ## level

#endif // _XSCRIPT_UTIL_LOG_HPP_
