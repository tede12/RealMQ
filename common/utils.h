#include <stdio.h>
#include <time.h>
#include <unistd.h>


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
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long) (ts.tv_sec) * 1000 + (long long) (ts.tv_nsec) / 1000000;
}

/**
 * @brief Get the current timestamp (precision: nanoseconds)
 *
 * @return long long
 */
long long get_current_time_nanos() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (long long)(ts.tv_sec) * 1000000000 + (long long)(ts.tv_nsec);
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