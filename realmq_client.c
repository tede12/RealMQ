#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <zmq.h>
#include <time.h>
#include "common/utils.h"
#include "common/config.h"
#include "common/zhelper.h"
#include "common/logger.h"
#include <signal.h>
#include <json-c/json.h>

// Global configuration
Config config;
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

    // use unique id for each message based time_millis
    json_object_object_add(json_msg, "id", json_object_new_double(getCurrentTimeValue(NULL)));
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

    printf("json_string: %s | current_len/len: %d/%lu\n", json_str, current_len, strlen(json_str));

    zmq_send(socket, json_str, strlen(json_str), 0);

    // Measure the arrival time
    zmq_recv(socket, message, sizeof(message), 0);

    logger(LOG_LEVEL_INFO, "Received message: %s", message);

    // timespec end_time = getCurrentTime();

    // received message...

}

// Function executed by client threads
void *client_thread(void *thread_id) {
    int thread_num = *(int *) thread_id;
    void *context = zmq_ctx_new();
    void *socket = zmq_socket(context, ZMQ_PUB);
    zmq_connect(socket, config.address);

    // Set a timeout for receive operations
    int timeout = 1000; // Timeout of 1000 milliseconds
    zmq_setsockopt(socket, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));

    // Wait for the specified time before starting to send messages
    s_sleep(config.client_action->sleep_starting_time);

    int messages_sent = 0;
    while (messages_sent < config.num_messages && !interrupted) {
        if (config.use_msg_per_time) {
            // Keep track of current time (for taking diff of a minute)
            timespec current_time = getCurrentTime();

            // Calculate the waiting time between messages in microseconds to not exceed messages per minute
            int interval_between_messages_us = 60000000 / config.msg_per_minute; // 60 million microseconds in a minute

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
                    printf("Interrupted\n");
                    break;
                }

                s_sleep(interval_between_messages_us); // Wait for the specified time between messages
            }

        } else {
            send_payload(socket, thread_num, messages_sent);
            messages_sent++;
        }

        // Check for interruption after sending a message
        if (interrupted) {
            printf("Interrupted\n");
            break;
        }
    }

    zmq_close(socket);
    zmq_ctx_destroy(context);

    printf("\n\n\n\n\nClosing with messages sent: %d, interrupt: %d\n", messages_sent, interrupted);
    return NULL;
}

int main() {
    // Register the interruption handling function
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = handle_interrupt;
    sigaction(SIGINT, &act, NULL);

    // Load the configuration
    logConfig logger_config = {
            .show_timestamp = 1,
            .show_thread_id = 1,
            .show_logger_name = 1,
            .log_to_console = 1,
            .log_level = LOG_LEVEL_INFO
    };

    Logger_init("realmq_client", &logger_config, &client_logger);

    if (read_config("../config.yaml", &config) != 0) {
        logger(LOG_LEVEL_ERROR, "Failed to read config.yaml");
        return 1;
    }

    // Print configuration
    logger(LOG_LEVEL_INFO, get_configuration(config));


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

    logger(LOG_LEVEL_INFO, "Execution Time: %.3f milliseconds", getElapsedTime(start_time, NULL));


    // Release the configuration
    release_config(&config);
    release_logger();

    return 0;
}
