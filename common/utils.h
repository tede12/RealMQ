//  =====================================================================
//  utils.h
//
//  Utility functions for the client and the server
//  =====================================================================

#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <libgen.h>

// Flag to indicate if keyboard interruption has been received
extern volatile sig_atomic_t interrupted;

typedef struct timespec timespec;

// Function to handle keyboard interrupt
void handle_interrupt(int sig);

// Function to create folder if it doesn't exist
char *create_if_not_exist_folder(char *folder_path);

// Function to get the current timestamp in seconds
time_t get_timestamp();

// Function to get the current date and time as a string
char *get_current_date_time();

// Function to get the current time in milliseconds
long long get_current_time_millis();

// Function to get the current time in nanoseconds
long long get_current_time_nanos();

// Function to get the current time as timespec
timespec getCurrentTime();

// Function to get the current time value as double
double getCurrentTimeValue(timespec *ts);

// Function to get the elapsed time between two timespec
double getElapsedTime(timespec start, timespec *end);

#endif //UTILS_H
