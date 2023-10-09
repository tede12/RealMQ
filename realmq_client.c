#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <zmq.h>
#include <time.h>
#include "common/utils.h"
#include "common/config.h"
#include "common/zhelper.h"
#include "common/logger.h"
#include <json-c/json.h>

// Global configuration
Config config;
Logger client_logger;

// Function executed by client threads
void *client_thread(void *thread_id) {
    int thread_num = *(int *) thread_id;
    void *context = zmq_ctx_new();
    void *socket = zmq_socket(context, ZMQ_PUB);
    zmq_connect(socket, config.address);

    // Wait for the specified time before starting to send messages
    s_sleep(config.client_action->sleep_starting_time);

    for (int i = 0; i < config.num_messages; i++) {
        // Create the message
        char message[256];
        sprintf(message, "Thread %d - Message %d", thread_num, i);

        // Calculate the send time
        timespec send_time = getCurrentTime();

        // Create a JSON object for the message
        json_object *json_msg = json_object_new_object();

        // use unique id for each message based time_millis
        json_object_object_add(json_msg, "id", json_object_new_double(getCurrentTimeValue(NULL)));
        json_object_object_add(json_msg, "message", json_object_new_string(message));
        json_object_object_add(json_msg, "send_time", json_object_new_double(getCurrentTimeValue(&send_time)));

        // Send the JSON message to the server
        const char *json_str = json_object_to_json_string(json_msg);
        zmq_send(socket, json_str, strlen(json_str), 0);

        // Measure the arrival time
        zmq_recv(socket, message, sizeof(message), 0);

        logger(LOG_LEVEL_INFO, "Received message: %s", message);

        // timespec end_time = getCurrentTime();

        // received message...
    }

    zmq_close(socket);
    zmq_ctx_destroy(context);
    return NULL;
}

int main() {
    // Load the configuration
    logConfig logger_config = {
            .show_timestamp = 1,
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
