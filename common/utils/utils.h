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
#include <pthread.h>
#include <assert.h>

// Flag to indicate if keyboard interruption has been received
extern volatile sig_atomic_t interrupted;


// List of IDs received since the last heartbeat
extern char **message_ids;
extern size_t num_message_ids;
extern const int MAX_RESPONSE_LENGTH;
extern char *IDS_SEPARATOR;
extern pthread_mutex_t message_ids_mutex; // Mutex to protect message_ids

// Function that process the IDs received since the last heartbeat and send them back to the client
void process_message_ids(void *responder, char *last_id);

// Function to get message ID from the list of IDs using the index
char *get_message_id(int index);

// Function to add a message ID to the list of IDs received since the last heartbeat
void add_message_id(const char *id_str);

// Function to process the missed message IDs
char **process_missed_message_ids(const char *buffer, size_t *missed_count);

// Function to handle keyboard interrupt
void handle_interrupt(int sig);


#endif //UTILS_H
