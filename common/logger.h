//  =====================================================================
//  logger.h
//
//  Logging functions
//  =====================================================================

#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <stdlib.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_INFO2 = 6,    // Colored info more visible
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_ERROR = 3,
    LOG_LEVEL_FATAL = 4,
    LOG_LEVEL_NONE = 5
} LogLevel;

typedef struct {
    int show_timestamp;   // If true, show timestamp
    int show_thread_id;   // If true, show thread id
    int show_logger_name; // If true, show logger name
    int log_to_console;   // If true, log to console otherwise log to syslog
    LogLevel log_level;   // Permit only messages with level <= log_level
} logConfig;

typedef struct Logger {
    char name[256];
    logConfig config;
} Logger;

// Global logger
extern Logger global_logger;

typedef struct Logger loggerType;

void logger(LogLevel level, const char *format, ...);

void Logger_init(const char *name, logConfig *config, Logger *my_logger);

void log_internal(int level, const char *format, va_list args);

void release_logger();

#endif //LOGGER_H
