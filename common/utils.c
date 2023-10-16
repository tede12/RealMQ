#include "utils.h"
#include "config.h"
#include "logger.h"

volatile sig_atomic_t interrupted = 0;
char *date_time = NULL;

/**
 * @brief Handle the keyboard interruption (Ctrl+C)
 * @param signal
 */
void handle_interrupt(int sig) {
    (void) sig; // Avoid unused parameter warning
    interrupted = 1;
    printf("\n[WARNING]: Interruzione da tastiera ricevuta (Ctrl+C)\n");
}

/**
 * @param folder_path
 * @return <folder_path> if the folder exists, otherwise create the folder and return <folder_path>
 */
char *create_if_not_exist_folder(char *folder_path) {
    struct stat st = {0};
    if (stat(folder_path, &st) == -1) {
        mkdir(folder_path, 0700);
    }
    return folder_path;
}

/**
 * @brief Create the full path for the statistics file
 * @param date_time
 * @return <config.stats_folder_path>/<date_time>_<config.protocol>_result.<config.stats_file_extension>
 */
char *create_stats_path() {
    // Generate one unique date time for each run
    if (date_time == NULL) {
        // Get the date + time for the filename
        date_time = malloc(20 * sizeof(char));
        if (date_time == NULL) {
            logger(LOG_LEVEL_ERROR, "Allocation error for date_time variable.");
            return NULL;
        }
        date_time = get_current_date_time();
    }

    // Get the file extension
    char *file_extension = config.use_json ? ".json" : ".csv";  // Added the dot (.) before the extensions

    // Calculate the total length of the final string
    // Lengths of the folder path, date_time, file extension, and additional characters
    // ("/", "_result.", and the null terminator)
    unsigned int totalLength =
            strlen(config.stats_folder_path)
            + strlen(date_time)
            + strlen(config.protocol)
            + strlen("_result")
            + strlen(file_extension) + 3;  // +3 for the '/' and the null terminator

    // Allocate memory for the full file path
    char *fullPath = (char *) malloc(totalLength * sizeof(char));

    if (fullPath == NULL) {
        logger(LOG_LEVEL_ERROR, "Errore di allocazione della memoria.");
        return NULL;
    }

    // Construct the full file path
    snprintf(
            fullPath, totalLength, "%s/%s_%s_result%s",
            config.stats_folder_path, date_time, config.protocol, file_extension);
    return fullPath;
}


/**
 * @brief Get the current timestamp (precision: seconds)
 *
 * @return time_t
 */
time_t get_timestamp() {
    time_t start_time;
    time(&start_time);
    return start_time;
}

/**
 * @brief Get the current date and time
 * @return char*
 */
char *get_current_date_time() {
    time_t rawtime;
    struct tm *timeinfo;
    char *buffer = (char *) malloc(80 * sizeof(char));

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, 80, "%d_%m_%Y_%I_%M_%S", timeinfo);
    return buffer;
}


/**
 * @brief Get the current timestamp (precision: milliseconds)
 *
 * @return long long
 */
long long get_current_time_millis() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long) (ts.tv_sec) * 1000 + (long long) (ts.tv_nsec) / 1000000;
}

/**
 * @brief Get the current timestamp (precision: nanoseconds)
 *
 * @return long long
 */
long long get_current_time_nanos() {
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (long long) (ts.tv_sec) * 1000000000 + (long long) (ts.tv_nsec);
}


/**
 * @brief Get the current time object
 * @return timespec
 */
timespec getCurrentTime() {
    timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        perror("clock_gettime");
        ts.tv_sec = 0;
        ts.tv_nsec = 0;
        printf("[getCurrentTime][Error]: getting current time\n");
    }
    return ts;
}

/**
 * @brief Get the current time value object
 * @param ts pass NULL to get current time value or pass a timespec struct to get the time value of that struct
 * @return double
 */
double getCurrentTimeValue(timespec *ts) {
    timespec localTs;

    if (ts == NULL) {
        localTs = getCurrentTime();
        ts = &localTs;
    }

    double timeValue = (double) ts->tv_sec + (double) ts->tv_nsec / 1e9;
    return timeValue;
}


/**
 * @brief Get the elapsed time object
 * @param start is a timespec struct
 * @param end pass NULL to get current time value or pass a timespec struct to get the time value of that struct
 * @return double
 */
double getElapsedTime(timespec start, timespec *end) {
    timespec localEnd;
    if (end == NULL) {
        localEnd = getCurrentTime();
        end = &localEnd;
    }
    return (double) (end->tv_sec - start.tv_sec) + (double) (end->tv_nsec - start.tv_nsec) / 1e9;
}

/**
 * Example of usage:
    time_t start_time = get_timestamp();
    long long start_time2 = get_current_time_nanos();
    printf("Timestamp now: %ld, %lld\n", start_time, start_time2);

    sleep(5);

    time_t end_time = get_timestamp();
    long long end_time2 = get_current_time_nanos();
    printf("Timestamp now: %ld, %lld\n", end_time, end_time2);

    printf("Time difference: %.3f, %.3f\n", difftime(end_time, start_time), difftime(end_time2, start_time2) / 1000.0);
*/
