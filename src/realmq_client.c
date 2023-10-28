#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <zmq.h>
#include<semaphore.h>
#include <time.h>
#include "utils/utils.h"
#include "core/config.h"
#include "core/zhelpers.h"
#include "core/logger.h"
#include "qos/accrual_detector.h"
#include "utils/time_utils.h"
#include "string_manip.h"

#ifdef REALMQ_VERSION

#include "qos/message_queue.h"

#endif

#include <signal.h>
#include <json-c/json.h>

// Global configuration
Logger client_logger;
size_t message_sent_correctly = 0;
sem_t mutex;  // semaphore to protect the counter


// Function for sending payloads to the server
void send_payload(void *socket, int thread_num, int messages_sent) {
    // Create the message (payload)
    char message[config.message_size];
    sprintf(message, "Thread %d - Message %d", thread_num, messages_sent);

    // Calculate the send time
    timespec send_time = getCurrentTime();

    // Create a JSON object for the message
    json_object *json_msg = json_object_new_object();

    char *message_id = generate_uuid();

    // Store the ID
    add_message_id(message_id);

    // use unique id for each message based time_millis
    json_object_object_add(json_msg, "id", json_object_new_string(message_id));
    json_object_object_add(json_msg, "message", json_object_new_string(message));
    json_object_object_add(json_msg, "send_time", json_object_new_double(getCurrentTimeValue(&send_time)));

    int current_len = strlen(json_object_to_json_string(json_msg));

    // Calculate the fill size, ensuring it's not negative
    int fill_size = config.message_size - current_len - 1 - 11;  // need to remove this 11 padding: |, "fill": ""|

    if (fill_size > 0) {
        char *fill_str = random_string(fill_size);

        // Add fill field to the message
        json_object_object_add(json_msg, "fill", json_object_new_string(fill_str));

        // Remember to free the allocated string
        free(fill_str);
    } else {
        // Handle the case when fill_size is <= 0
        logger(LOG_LEVEL_ERROR, "Not enough space for fill string!\n");
    }


    // Send the JSON message to the server
    const char *json_str = json_object_to_json_string(json_msg);

#ifdef REALMQ_VERSION
    zmq_send_group(socket, get_group(MAIN_GROUP), json_str, 0);

//    // Enqueue the message after it's sent
//    enqueue_message(message_id, send_time);
#else
    zmq_send(socket, json_str, strlen(json_str), 0);

    // ----- this things are implemented in the message_queue -----

    // Check if the message was sent successfully
    zmq_recv(socket, message, sizeof(message), 0);

#endif
    logger(LOG_LEVEL_INFO, "Sent message with ID: %s", message_id);

}

#ifdef REALMQ_VERSION

// Function to be executed by the response handling thread
void *response_handler(void *arg) {
    void *socket = (void *) arg;

    if (socket == NULL) {
        logger(LOG_LEVEL_ERROR, "Responder Socket is NULL");
        return NULL;
    }

    while (!interrupted) {
        char buffer[MAX_RESPONSE_LENGTH];


        // Receive response from server
        int recv_len = zmq_recv(socket, buffer, MAX_RESPONSE_LENGTH - 1, 0); // -1 to allow space for null-terminator
        if (recv_len > 0) {
            // Ensure null-termination of the response string
            buffer[recv_len] = '\0';

            // Process the response and delete the message IDs from the global list
            size_t missed_count = 0;

            char **missed_ids = process_missed_message_ids(buffer, &missed_count);
            logger(LOG_LEVEL_WARN, "Missed messages: %zu", missed_count);
            // todo resend messages missed

        } else if (errno == EAGAIN) {
            // No response received, try again
            continue;
        } else {
            // Some other error occurred
            logger(LOG_LEVEL_ERROR, "Error receiving response from server: %s", strerror(errno));
            continue;
        }
    }


    return NULL;
}

#endif

// Function executed by client threads
void *client_thread(void *thread_id) {
    signal(SIGINT, handle_interrupt); // Register the interruption handling function

    int thread_num = *(int *) thread_id;
    void *context = create_context();

    // Create the socket for sending messages PUB/RADIO
    void *socket = create_socket(
            context,
            get_zmq_type(CLIENT),
            get_address(MAIN_ADDRESS),
            config.signal_msg_timeout,
            NULL
    );

    // Wait for the specified time before starting to send messages
    s_sleep(config.client_action->sleep_starting_time);

    int messages_sent = 0;
    while (messages_sent < config.num_messages && !interrupted) {


        // add nano sleep to avoid sending messages too fast
        struct timespec ts;
        ts.tv_sec = 0;

        // random number between 0 and 100000
        // 100000; // 0.01 ms
        ts.tv_nsec = rand() % 10000000; // 0.01 ms
        nanosleep(&ts, NULL);
        // --------------------------------------------------

#ifdef REALMQ_VERSION
        // Send a heartbeat before starting to send messages
        send_heartbeat(socket, get_group(MAIN_GROUP), false);  // Refer to common/accrual_detector.c
#endif


        if (config.use_msg_per_minute) {
            // Keep track of current time (for taking diff of a minute)
            timespec current_time = getCurrentTime();

            // Calculate the waiting time between messages in microseconds to not exceed messages per minute
//            int interval_between_messages_us = 60000000 / config.msg_per_minute; // 60 million microseconds in a minute

            for (int i = 0; i < config.msg_per_minute && messages_sent < config.num_messages && !interrupted; i++) {
                // Calculate diff between current time and start time
                double diff = getElapsedTime(current_time, NULL);
                if (diff >= 60) {
                    // More than a minute has passed, these messages are out of scope
                    logger(LOG_LEVEL_WARN, "Message %d is out of scope", messages_sent);
                }

                send_payload(socket, thread_num, messages_sent);
                messages_sent++;

                // Check for interruption before sleeping
                if (interrupted) {
                    logger(LOG_LEVEL_INFO, "***Exiting client thread.");
                    break;
                }

                // TODO: maybe is better to check for heartbeat after sending a batch of messages (i % SOME_VALUE == 0)
                // s_sleep(interval_between_messages_us); // Wait for the specified time between messages
            }

        } else {
            send_payload(socket, thread_num, messages_sent);
            messages_sent++;
        }

#ifndef REALMQ_VERSION  // ONLY FOR TCP
        // After sending a batch of messages, check if the socket is still connected in TCP
        try_reconnect(context, &socket, get_address(MAIN_ADDRESS), get_zmq_type(CLIENT));
#endif

    }

#ifdef REALMQ_VERSION
    // Send last heartbeat for synchronization before exiting
    send_heartbeat(socket, get_group(MAIN_GROUP), true);  // Refer to common/accrual_detector.c
#endif

    zmq_close(socket);
    zmq_ctx_destroy(context);

    if (interrupted)
        logger(LOG_LEVEL_INFO, "***Exiting client thread.");

    // add message_sent to the global counter
    sem_wait(&mutex);
    message_sent_correctly += messages_sent;
    sem_post(&mutex);

    return NULL;
}

int main() {
    // Load the configuration
    logConfig logger_config = {
            .show_timestamp = 1,
            .show_thread_id = 1,
            .show_logger_name = 1,
            .log_to_console = 1,
            .log_level = LOG_LEVEL_INFO
    };

    Logger_init("realmq_client", &logger_config, &client_logger);

    if (read_config("../config.yaml") != 0) {
        logger(LOG_LEVEL_ERROR, "Failed to read config.yaml");
        return 1;
    }

    // Print configuration
    get_configuration();

#ifdef REALMQ_VERSION
    //    // Initialize message queue
    //    initialize_message_queue();

    // Initialize the RESPONDER handling thread
    void *response_context = create_context();
    void *response_socket = create_socket(
            response_context,
            ZMQ_DISH,
            get_address(RESPONDER),
            config.signal_msg_timeout,
            get_group(RESPONDER_GROUP)
    );
#endif
#ifdef QOS_RETRANSMISSION
    // Subscribe to all messages
    pthread_t response_thread;
    if (pthread_create(&response_thread, NULL, response_handler, response_socket)) {
        fprintf(stderr, "Error creating thread\n");
        return 1;
    }
#endif

    // Print the initial configuration for the client
    timespec start_time = getCurrentTime();
    logger(LOG_LEVEL_DEBUG, "Start Time: %.3f", getCurrentTimeValue(&start_time));

    // Client threads
    pthread_t clients[config.num_threads];
    int thread_ids[config.num_threads];
    for (int i = 0; i < config.num_threads; i++) {
        thread_ids[i] = i;
        pthread_create(&clients[i], NULL, client_thread, &thread_ids[i]);
    }
    // Wait for client threads to finish
    for (int i = 0; i < config.num_threads; i++) {
        pthread_join(clients[i], NULL);
    }

    logger(LOG_LEVEL_DEBUG, "Execution Time: %.3f ms (+ %d ms of sleep starting time)",
           getElapsedTime(start_time, NULL), config.server_action->sleep_starting_time);
    logger(LOG_LEVEL_DEBUG, "Messages sent correctly: %ld", message_sent_correctly);

#ifdef REALMQ_VERSION
    //    // Finalize message queue
    //    finalize_message_queue();

    // Finalize the RESPONDER handling thread
    zmq_close(response_socket);
    zmq_ctx_destroy(response_context);

#endif

#ifdef QOS_RETRANSMISSION
    pthread_join(response_thread, NULL);
#endif

    // Release the configuration
    release_config();
    release_logger();

    return 0;
}
