#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <zmq.h>
#include <time.h>
#include <unistd.h> // for sleep function
#include <signal.h>
#include <json-c/json.h>
#include "common/utils.h"
#include "common/config.h"
#include "common/zhelpers.h"
#include "common/logger.h"


// Global configuration
Logger client_logger;

// Variables for periodic statistics saving
json_object *json_messages = NULL; // Added to store all messages

// Function for handling received messages
void handle_message(const char *message) {
    // Implement logic for handling received messages
    // In this example, we are just printing the message for demonstration purposes
//    logger(LOG_LEVEL_INFO, "Received message: %s", message);
}

// Function for processing received JSON message and updating statistics
void process_json_message(const char *json_str, double recv_time) {
    // "{ \"message\": \"Thread 20 - Message 400\", \"send_time\": 358913413 }"
    json_object *json_msg = json_tokener_parse(json_str);
    if (json_msg) {
        // Add the message to the statistical data

        // Create a JSON object for the message
        json_object *jobj = json_object_new_object();
        json_object_object_add(jobj, "id", json_object_object_get(json_msg, "id"));
//        json_object_object_add(jobj, "message", json_object_object_get(json_msg, "message"));
        json_object_object_add(jobj, "send_time", json_object_object_get(json_msg, "send_time"));
        json_object_object_add(jobj, "recv_time", json_object_new_double(recv_time));
        json_object_array_add(json_messages, jobj);
    }
}


// Function executed by the server
void *server_thread(void *args) {
    signal(SIGINT, handle_interrupt); // Register the interruption handling function

    void *context = create_context();
    void *receiver = create_socket(
            context, get_zmq_type(SERVER),
            get_address(MAIN_ADDRESS),
            config.signal_msg_timeout,
            get_group(MAIN_GROUP)
    );

#ifdef REALMQ_VERSION
    // Responder socket
    void *responder = create_socket(
            context, ZMQ_RADIO,
            get_address(RESPONDER),
            config.signal_msg_timeout,
            NULL
    );
#endif

    // Wait for the specified time before starting to receive messages
    s_sleep(config.server_action->sleep_starting_time);

    int messages_received = 0; // Counter for received messages

    while (!interrupted) {
        char message[config.message_size + 64];
        int size = zmq_recv(receiver, message, sizeof(message), 0); // zmq_recv will return after timeout if no message
        if (size == -1 && errno == EAGAIN) {
            // In this case no message received or timeout occurred
            continue;
        }
        double recv_time = getCurrentTimeValue(NULL);

        // Process the received message
        handle_message(message);

        // Process the received JSON message and update statistics
        process_json_message(message, recv_time);

        // Increment the received messages counter
        messages_received++;

        // Extract the ID from the message
        json_object *json_msg = json_tokener_parse(message);
        if (json_msg) {
#ifdef REALMQ_VERSION
            // This part send with the socket RESPONDER back the ID of the message to the client
            json_object *id_obj;
            if (json_object_object_get_ex(json_msg, "id", &id_obj)) {
                const char *id_str = json_object_get_string(id_obj);

                // Send the ID back as a response
//                zmq_send_group(responder, get_group(RESPONDER_GROUP), id_str, 0);
            }
#endif
            json_object_put(json_msg);  // free json object memory
        }
    }

    logger(LOG_LEVEL_DEBUG, "Received messages: %d", messages_received);

    zmq_close(receiver);
#ifdef REALMQ_VERSION
    zmq_close(responder);
#endif
    zmq_ctx_destroy(context);

    if (interrupted)
        logger(LOG_LEVEL_DEBUG, "***Exiting server thread.");
    return NULL;
}

// Function for saving statistics to a file
void save_stats_to_file() {
    if (json_messages == NULL || json_object_array_length(json_messages) == 0) {
        return;
    }

    char *fullPath = create_stats_path();

    // Extract the folder path
    char *folder = strdup(fullPath); // Duplicate fullPath because dirname can modify the input argument
    char *dir = dirname(folder);
    create_if_not_exist_folder(dir);

    logger(LOG_LEVEL_DEBUG, "Saving statistics to file: %s", fullPath);

    FILE *file = fopen(fullPath, "w");
    free(fullPath);

    if (file == NULL) {
        logger(LOG_LEVEL_ERROR, "Impossibile aprire il file per la scrittura.");
        return;
    }

    if (config.use_json) {
        json_object *jobj = json_object_new_object();
        json_object_object_add(jobj, "messages", json_messages);

        const char *json_data = json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PRETTY);
        fprintf(file, "%s\n", json_data);

        // Clear json_messages
        json_messages = json_object_new_array(); // Added to store all messages
    } else {
        // Write the CSV header
        fprintf(file, "id,num,diff\n");

        unsigned long array_len = json_object_array_length(json_messages);
        for (unsigned int i = 0; i < array_len; i++) {
            json_object *json_msg = json_object_array_get_idx(json_messages, i);

            double diff = json_object_get_double(json_object_object_get(json_msg, "recv_time")) -
                          json_object_get_double(json_object_object_get(json_msg, "send_time"));
            if (json_msg) {
                fprintf(file, "%s,%d,%f\n",
                        json_object_get_string(json_object_object_get(json_msg, "id")),
                        i + 1,
                        diff
                );
            }
        }
    }
    fclose(file);
}

// Function for handling periodic statistics saving
void *stats_saver_thread(void *args) {
    while (!interrupted) {
        // Wait for the specified time before the next save
        sleep(config.save_interval_seconds);

        // Save statistics to a file
        save_stats_to_file();
    }
    if (interrupted)
        logger(LOG_LEVEL_DEBUG, "***Exiting stats saver thread.");
    return NULL;
}

int main() {
    // Load the configuration
    logConfig logger_config = {
            .show_timestamp = 1,
            .show_logger_name = 1,
            .show_thread_id = 1,
            .log_to_console = 1,
            .log_level = LOG_LEVEL_INFO
    };

    Logger_init("realmq_sever", &logger_config, &client_logger);


    if (read_config("../config.yaml") != 0) {
        logger(LOG_LEVEL_ERROR, "Failed to read config.yaml");
        return 1;
    }

    // Print configuration
    logger(LOG_LEVEL_DEBUG, get_configuration());

    // Initialize JSON statistics
    json_messages = json_object_new_array();

    // Initialize the thread for periodic statistics saving
    pthread_t stats_saver;
    pthread_create(&stats_saver, NULL, stats_saver_thread, NULL);

    // Initialize the server thread
    pthread_t server;
    pthread_create(&server, NULL, server_thread, NULL);

    // Wait for the server thread to finish
    pthread_join(server, NULL);

    // Terminate the thread for periodic statistics saving
    pthread_cancel(stats_saver);
    pthread_join(stats_saver, NULL);

    // Save final statistics to a file
    save_stats_to_file();

    // Deallocate json_messages before exiting
    json_object_put(json_messages);

    // Release the configuration
    release_config();
    release_logger();

    return 0;
}
