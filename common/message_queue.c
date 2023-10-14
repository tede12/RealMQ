#include "message_queue.h"
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h> // For sleep
#include "logger.h"
#include "utils.h"

// The flag to control the background thread's loop
volatile bool keep_running = true;

// Queue for messages awaiting response
static MessageData *message_queue = NULL;
static int queue_size = 0;
static int queue_capacity = 100; // Adjust as needed

// Mutex for message queue
static pthread_mutex_t queue_mutex;

static pthread_t timeout_check_thread;


// Placeholder for your logging function
void log_error(const char *error_message) {
    // Implement your error logging logic here, for instance, writing to a file or sending to a logging system
    printf("Error: %s\n", error_message);
}

// Placeholder for your message resend function
void resend_message(MessageData *message) {
    // Here goes your logic for resending the message. This might involve network connection,
    // message serialization, etc.
    printf("Resending message with ID: %f\n", message->id);
}

// Complete handle_timeout function
void handle_timeout(MessageData *message) {
    // Print a timeout message
    printf("Timeout occurred for message with ID: %f\n", message->id);

    // Log the timeout event
    char error_message[100];
    snprintf(error_message, 100, "Timeout occurred for message with ID: %f", message->id);
    log_error(error_message);

    // Try to resend the message
    resend_message(message);

    // Notify that the message has been handled
    printf("Message with ID %f has been handled after a timeout.\n", message->id);
}

void initialize_message_queue() {
    pthread_mutex_init(&queue_mutex, NULL);
    message_queue = (MessageData *) malloc(queue_capacity * sizeof(MessageData));
    assert(message_queue != NULL); // Ensure malloc was successful

    // Declare and initialize the thread for checking the message queue
    if (pthread_create(&timeout_check_thread, NULL, check_queue_and_process_responses, NULL)) {
        fprintf(stderr, "Error creating timeout check thread\n");
    }
}

void finalize_message_queue() {
    // Once client threads are done, you can stop the timeout_check_thread
    keep_running = false; // This will signal the thread to exit
    pthread_join(timeout_check_thread, NULL); // Wait for the timeout check thread to finish

    free(message_queue);
    pthread_mutex_destroy(&queue_mutex);
}

void enqueue_message(double id, struct timespec send_time) {
    pthread_mutex_lock(&queue_mutex);

    // Check if the queue is full
    if (queue_size >= queue_capacity) {
        // Queue is full, increase capacity
        queue_capacity *= 2; // Double the capacity
        message_queue = realloc(message_queue, queue_capacity * sizeof(MessageData));
        assert(message_queue != NULL); // Ensure realloc was successful
    }

    // Add message data to the queue
    message_queue[queue_size].id = id;
    message_queue[queue_size].send_time = send_time;
    queue_size++;

    pthread_mutex_unlock(&queue_mutex);
}

void dequeue_message(double id) {
    pthread_mutex_lock(&queue_mutex);

    // Find the message with the given ID and remove it
    for (int i = 0; i < queue_size; i++) {
        if (message_queue[i].id == id) {
            // Shift the remaining elements down
            for (int j = i; j < queue_size - 1; j++) {
                message_queue[j] = message_queue[j + 1];
            }
            queue_size--;
            break;
        }
    }

    pthread_mutex_unlock(&queue_mutex);
}

void *check_queue_and_process_responses(void *args) {
    while (keep_running && !interrupted) {
        pthread_mutex_lock(&queue_mutex);

        int i = 0;
        while (i < queue_size && keep_running && !interrupted) {
            struct timespec current_time;
            clock_gettime(CLOCK_REALTIME, &current_time);

            double time_diff = (double) (current_time.tv_sec - message_queue[i].send_time.tv_sec) +
                               (double) (current_time.tv_nsec - message_queue[i].send_time.tv_nsec) / 1E9;

            if (time_diff > RESPONSE_TIMEOUT) {
                handle_timeout(&message_queue[i]); // Pass the address of message_queue[i] here

                dequeue_message(message_queue[i].id); // This should be after handling the timeout, not before
                continue; // continue since the list is now shifted
            }
            i++;
        }

        pthread_mutex_unlock(&queue_mutex);
        sleep(1);
    }

    if (interrupted) {
        logger(LOG_LEVEL_INFO, "***Exiting message queue thread.");
    }

    return NULL;
}
