#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <propeller2.h>

#ifndef __WORKSPACE__
#define __WORKSPACE__ ""
#endif

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

extern int _stdio_debug_lock;

#define DEBUG_PRINTF_RAW(color, fmt, ...)                                                                                  \
    while (!_locktry(_stdio_debug_lock))                                                                                   \
    {                                                                                                                      \
    }                                                                                                                      \
    fprintf(stdout, color "%0.3f - %s:%d: " fmt ANSI_COLOR_RESET, _getus() / 1000000.0f, __FILE__, __LINE__, __VA_ARGS__); \
    _lockrel(_stdio_debug_lock);

#if defined(_DEBUG_PRINTF_RAW)
#define DEBUG_MESSAGE(type, color, fmt, ...) \
    DEBUG_PRINTF_RAW(ANSI_COLOR_CYAN, fmt, __VA_ARGS__);
#elif defined(_DEBUG_PRINTF_RAW)
#define DEBUG_MESSAGE(fmt, ...) DEBUG_PRINTF_RAW(ANSI_COLOR_CYAN, fmt, __VA_ARGS__);
#define DEBUG_MESSAGE(...)
#endif

#if defined(_DEBUG_WARNING)
#define DEBUG_WARNING(fmt, ...) DEBUG_MESSAGE(APP_NOTIFICATION_TYPE_WARNING, ANSI_COLOR_YELLOW, fmt, __VA_ARGS__);
#endif

#if defined(_DEBUG_INFO)
#define DEBUG_INFO(fmt, ...) DEBUG_MESSAGE(APP_NOTIFICATION_TYPE_INFO, ANSI_COLOR_GREEN, fmt, __VA_ARGS__);
#else
#define DEBUG_INFO(...)
#endif

#if defined(_DEBUG_ERROR)
#define DEBUG_ERROR(fmt, ...) DEBUG_MESSAGE(APP_NOTIFICATION_TYPE_ERROR, ANSI_COLOR_RED, fmt, __VA_ARGS__);
#else
#define DEBUG_ERROR(...)
#endif

#endif
