#include "time_utils.h"

/**
 * @brief Sleep for a random time between min_ms and max_ms
 *
 * @param min_ms
 * @param max_ms
 */
void rand_sleep(int min_ms, int max_ms) {
    // Init random seed
    srand(time(NULL));

    int range = max_ms - min_ms + 1;
    // Generate random number between min_ms and max_ms
    int sleep_time = min_ms + (rand() % range);

    usleep(sleep_time * 1000);
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
timespec get_current_time() {
    timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        perror("clock_gettime");
        ts.tv_sec = 0;
        ts.tv_nsec = 0;
        printf("[get_current_time][Error]: getting current time\n");
    }
    return ts;
}

/**
 * @brief Get the current time value object
 * @param ts pass NULL to get current time value or pass a timespec struct to get the time value of that struct
 * @return double
 */
double get_current_time_value(timespec *ts) {
    timespec localTs;

    if (ts == NULL) {
        localTs = get_current_time();
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
double get_elapsed_time(timespec start, timespec *end) {
    timespec localEnd;
    if (end == NULL) {
        localEnd = get_current_time();
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
