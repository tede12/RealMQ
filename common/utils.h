#include <stdio.h>
#include <time.h>
#include <unistd.h>

typedef struct timespec timespec;


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

    double timeValue = (double)ts->tv_sec + (double)ts->tv_nsec / 1e9;
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
    return (double)(end->tv_sec - start.tv_sec) + (double)(end->tv_nsec - start.tv_nsec) / 1e9;
}

//int main() {
//    time_t start_time = get_timestamp();
//    long long start_time2 = get_current_time_nanos();
//    printf("Timestamp now: %ld, %lld\n", start_time, start_time2);
//
//    sleep(5);
//
//    time_t end_time = get_timestamp();
//    long long end_time2 = get_current_time_nanos();
//    printf("Timestamp now: %ld, %lld\n", end_time, end_time2);
//
//    printf("Time difference: %.3f, %.3f\n", difftime(end_time, start_time), difftime(end_time2, start_time2) / 1000.0);
//
//    return 0;
//}