#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <zmq.h>
#include <time.h>
#include "common/utils.h"
#include "common/config.h"
#include "common/zhelper.h"
#include "common/logger.h"

#define REALMQ_VERSION
#ifdef REALMQ_VERSION

#include "common/qos.h"
#include "common/message_queue.h"

#endif

#include <signal.h>
#include <json-c/json.h>

// Global configuration
Logger client_logger;

// Function that returns a random string of the given size
char *random_string(unsigned int string_size) {
    // Generate a random ascii character between '!' (0x21) and '~' (0x7E) excluding '"'
    char *random_string = malloc((string_size + 1) * sizeof(char)); // +1 for the null terminator
    for (int i = 0; i < string_size; i++) {
        do {
            random_string[i] = (char) (rand() % (0x7E - 0x21) + 0x21); // start from '!' to exclude space

        } while (
            // regenerate if the character is a double quote or a backslash or slash that depends on the
            // escape character (and could break the size of the message)
                random_string[i] == 0x22 || random_string[i] == 0x5C || random_string[i] == 0x2F);
    }
    random_string[string_size] = '\0'; // Null terminator
    return random_string;
}

// Function for sending payloads to the server
void send_payload(void *socket, int thread_num, int messages_sent) {
    // Create the message (payload)
    char message[config.message_size];
    sprintf(message, "Thread %d - Message %d", thread_num, messages_sent);

    // Calculate the send time
    timespec send_time = getCurrentTime();

    // Create a JSON object for the message
    json_object *json_msg = json_object_new_object();

    double message_id = getCurrentTimeValue(NULL);

    // use unique id for each message based time_millis
    json_object_object_add(json_msg, "id", json_object_new_double(message_id));
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

    zmq_send(socket, json_str, strlen(json_str), 0);

#ifdef REALMQ_VERSION

    logger(LOG_LEVEL_INFO, "Sent message with ID: %f", message_id);

    // Enqueue the message after it's sent
    enqueue_message(message_id, send_time);
    return;
#else

    // ----- this things are implemented in the message_queue -----

    // Check if the message was sent successfully
    zmq_recv(socket, message, sizeof(message), 0);

    logger(LOG_LEVEL_INFO, "Received message: %s", message);

    // timespec end_time = getCurrentTime();

    // received message...
#endif

}

#ifdef REALMQ_VERSION

// Function to be executed by the response handling thread
void *response_handler(void *socket) {
    // TODO: need to check the message payload for response (64 should be enough anyway)
    int recv_len = 64;
    char response[recv_len];

    while (!interrupted) {
        if (zmq_recv(socket, response, recv_len, 0) > 0) {
            double id = atof(response);
            dequeue_message(id);
            logger(LOG_LEVEL_INFO, "Received response: %s", response);
        }
    }
    return NULL;
}

#endif

// Function executed by client threads
void *client_thread(void *thread_id) {
    signal(SIGINT, handle_interrupt); // Register the interruption handling function

    int thread_num = *(int *) thread_id;
    void *context = zmq_ctx_new();
    void *socket = zmq_socket(context, get_zmq_type(CLIENT));
    zmq_connect(socket, get_address(MAIN_ADDRESS));

    // Set a timeout for receive operations
    zmq_setsockopt(socket, ZMQ_RCVTIMEO, &config.signal_msg_timeout, sizeof(config.signal_msg_timeout));

#ifdef REALMQ_VERSION
    // Set a timeout for sending operations (for heartbeats)
    zmq_setsockopt(socket, ZMQ_SNDTIMEO, &config.signal_msg_timeout, sizeof(config.signal_msg_timeout));
#endif

    // Wait for the specified time before starting to send messages
    s_sleep(config.client_action->sleep_starting_time);

    int messages_sent = 0;
    while (messages_sent < config.num_messages && !interrupted) {
#ifdef REALMQ_VERSION
        // Send a heartbeat before starting to send messages
        send_heartbeat(socket);
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

#ifdef REALMQ_VERSION
        // After sending a batch of messages, check if the socket is still connected
        try_reconnect(context, &socket, get_address(MAIN_ADDRESS), get_zmq_type(CLIENT));
#endif

    }

    zmq_close(socket);
    zmq_ctx_destroy(context);

    if (interrupted)
        logger(LOG_LEVEL_INFO, "***Exiting client thread.");

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
    logger(LOG_LEVEL_INFO, get_configuration());

#ifdef REALMQ_VERSION
    // Initialize message queue
    initialize_message_queue();

    // Initialize the response handling thread
    void *response_context = zmq_ctx_new();
    void *response_socket = zmq_socket(response_context, ZMQ_SUB);
    zmq_connect(response_socket, get_address(RESPONDER));
    // Set timeout for receive operations
    zmq_setsockopt(response_socket, ZMQ_RCVTIMEO, &config.signal_msg_timeout, sizeof(config.signal_msg_timeout));

    // Subscribe to all messages
    pthread_t response_thread;
    if (pthread_create(&response_thread, NULL, response_handler, response_socket)) {
        fprintf(stderr, "Error creating thread\n");
        return 1;
    }
#endif

    // Print the initial configuration for the client
    timespec start_time = getCurrentTime();
    logger(LOG_LEVEL_INFO, "Start Time: %.3f", getCurrentTimeValue(&start_time));

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

    logger(LOG_LEVEL_INFO, "Execution Time: %.3f ms (+ %d ms of sleep starting time)",
           getElapsedTime(start_time, NULL), config.server_action->sleep_starting_time);

#ifdef REALMQ_VERSION
    // Finalize message queue
    finalize_message_queue();
#endif

    // Release the configuration
    release_config();
    release_logger();

    return 0;
}
