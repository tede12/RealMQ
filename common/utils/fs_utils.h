//  =====================================================================
//  fs_utils.h
//
//  Utility functions for File System
//  =====================================================================

#ifndef FS_UTILS_H
#define FS_UTILS_H

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
#include <stdbool.h>
#include <json-c/json.h>
#include "qos/dynamic_array.h"


// Get the date + time for the filename
extern char *date_time;

extern pthread_mutex_t json_mutex; // Mutex for json_messages
extern json_object *g_json_messages; // Global JSON object to store the received messages


// Function to save the statistics to a file
void save_stats_to_file(json_object **json_messages_ptr);

// Function to check if a file exists
bool check_file_exists(char *file_path);

// Function to create folder if it doesn't exist
char *create_if_not_exist_folder(char *folder_path);

// Function to create the full path for the statistics file
char *create_stats_path();

// Function to release the date_time variable
void release_date_time();

// Function to initialize the global JSON object
void init_json_messages();

// Free the global JSON object
void release_json_messages();

// Add a new message to the global JSON object
void process_json_message(Message *message);


#endif //FS_UTILS_H
