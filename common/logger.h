#ifndef REALMQ_LOGGER_H
#define REALMQ_LOGGER_H

#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <stdlib.h>

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL,
    LOG_LEVEL_NONE
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
void release_logger();

#endif //REALMQ_LOGGER_H
