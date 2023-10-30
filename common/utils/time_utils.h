//  =====================================================================
//  time_utils.h
//
//  Utility functions for the client and the server
//  =====================================================================

#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <libgen.h>
#include <pthread.h>
#include <assert.h>


typedef struct timespec timespec;


// Function to get the current timestamp in seconds
time_t get_timestamp();

// Function to get the current date and time as a string
char *get_current_date_time();

// Function to get the current time in milliseconds
long long get_current_time_millis();

// Function to get the current time in nanoseconds
long long get_current_time_nanos();

// Function to get the current time as timespec
timespec get_current_time();

// Function to get the current time value as double
double get_current_time_value(timespec *ts);

// Function to get the elapsed time between two timespec
double get_elapsed_time(timespec start, timespec *end);

#endif //TIME_UTILS_H
