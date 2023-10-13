// message_queue.h
#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include <time.h>
#include <stdbool.h>

#define RESPONSE_TIMEOUT 5.0 // Define your timeout value here, in seconds

typedef struct {
    double id;
    struct timespec send_time;
} MessageData;

void initialize_message_queue();

void finalize_message_queue();

void enqueue_message(double id, struct timespec send_time);

void dequeue_message(double id);

void *check_queue_and_process_responses(void *args);

void handle_timeout(MessageData *message);

#endif //MESSAGE_QUEUE_H
