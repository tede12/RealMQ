#include "logger.h"

// Global logger
Logger global_logger;

/**
 * Log a message with the given level and format
 * @param level
 * @param format
 * @param args
 */
void log_internal(int level, const char *format, va_list args) {
    char buffer[1024];
    char prefix[256] = "";
    time_t current_time;
    struct tm *time_info;
    struct timespec ts;

    if (global_logger.config.show_timestamp) {
        clock_gettime(CLOCK_REALTIME, &ts);
        current_time = time(NULL);
        time_info = localtime(&current_time);
        snprintf(prefix, sizeof(prefix), "[%02d:%02d:%02d:%04ld]", time_info->tm_hour, time_info->tm_min,
                 time_info->tm_sec, ts.tv_nsec / 1000000);
    }

    if (global_logger.config.show_logger_name) {
        strncat(prefix, "[", sizeof(prefix) - strlen(prefix) - 1);
        strncat(prefix, global_logger.name, sizeof(prefix) - strlen(prefix) - 1);
        strncat(prefix, "]", sizeof(prefix) - strlen(prefix) - 1);
    }

    vsnprintf(buffer, sizeof(buffer), format, args);

    char final_msg[1280];
    char *string_level;
    char *color;
    switch (level) {

        case LOG_LEVEL_INFO:
            string_level = strdup("INFO");
            color = NULL;
            break;
        case LOG_LEVEL_INFO2:
            string_level = strdup("INFO");
            color = ANSI_COLOR_BLUE;
            break;
        case LOG_LEVEL_ERROR:
            string_level = strdup("ERROR");
            color = ANSI_COLOR_RED;
            break;
        case LOG_LEVEL_DEBUG:
            string_level = strdup("DEBUG");
            color = ANSI_COLOR_GREEN;
            break;
        case LOG_LEVEL_WARN:
            string_level = strdup("WARN");
            color = ANSI_COLOR_YELLOW;
            break;
        case LOG_LEVEL_FATAL:
            string_level = strdup("FATAL");
            color = ANSI_COLOR_MAGENTA;
            break;
        default:
            string_level = strdup("UNKNOWN");
            color = NULL;
            break;
    }

    char thread_id[18] = ""; // Default value is an empty string
    if (global_logger.config.show_thread_id) {
        snprintf(thread_id, sizeof(thread_id), "[%ld]", (long int) pthread_self());
    }

    // Create the final message
    snprintf(final_msg, sizeof(final_msg), "%s[%s]%s: %s", prefix, string_level, thread_id, buffer);

    if (global_logger.config.log_to_console) {
        // Print to console
        if (color != NULL) {
            printf("%s[%s%s%s]%s%s%s%s\n", prefix, color, string_level, ANSI_COLOR_RESET, thread_id, color, buffer,
                   ANSI_COLOR_RESET);
        } else {
            printf("%s\n", final_msg);
        }
    }

    // Print to syslog if the level is >= to the log level
    if (level >= global_logger.config.log_level) {
        syslog(level, "%s", final_msg);
    }

    // strdup() allocates memory using malloc() and needs to be freed
    free(string_level);
}

void logger(LogLevel level, const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_internal(level, format, args);
    va_end(args);
}

void Logger_init(const char *name, logConfig *config, loggerType *my_logger) {
    if (my_logger == NULL) {
    } else {
        global_logger = *my_logger;
    }

    strncpy(global_logger.name, name, sizeof(global_logger.name) - 1);
    global_logger.config = *config;
    openlog(name, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
}

// Close the logger
void release_logger() {
    closelog();
}

/**
    * Example usage:

    logConfig config = {
            .show_timestamp = 1,
            .show_logger_name = 1,
            .log_to_console = 1,
            .log_level = LOG_LEVEL_INFO
    };

    Logger_init("logger_name", &config);

    logger(LOG_LEVEL_INFO, "Ciao %s!", "Mondo");
    logger(LOG_LEVEL_ERROR, "Questo è un messaggio di errore");
    logger(LOG_LEVEL_DEBUG, "Questo messaggio di debug non verrà mostrato");

    release_logger();

*/
