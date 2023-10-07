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
#include "common/zhelper.h"
#include <stdbool.h>

// Global configuration
Config config;

// Flag to indicate if keyboard interruption has been received
volatile bool interrupted = false;

// Function for handling interruption
void handle_interrupt(int signal) {
    if (signal == SIGINT) {
        interrupted = true;
        printf("\nKeyboard interruption received (Ctrl+C)\n");
    }
    exit(0);
}

// Variables for periodic statistics saving
json_object *json_messages = NULL; // Added to store all messages

// Function for handling received messages
void handle_message(const char *message) {
    // Implement logic for handling received messages
    // In this example, we are just printing the message for demonstration purposes
    printf("Received message: %s\n", message);
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

    void *context = zmq_ctx_new();
    void *socket = zmq_socket(context, ZMQ_SUB);
    zmq_bind(socket, config.address);
    zmq_setsockopt(socket, ZMQ_SUBSCRIBE, "", 0);

    // Wait for the specified time before starting to receive messages
    s_sleep(config.server_action->sleep_starting_time);

    int messages_received = 0; // Counter for received messages

    while (!interrupted) {
        char message[512];
        zmq_recv(socket, message, sizeof(message), 0);
        double recv_time = getCurrentTimeValue(NULL);

        // Process the received message
        handle_message(message);

        // Process the received JSON message and update statistics
        process_json_message(message, recv_time);

        // Increment the received messages counter
        messages_received++;
    }

    printf("Received messages: %d\n", messages_received);

    zmq_close(socket);
    zmq_ctx_destroy(context);
    return NULL;
}

// Function for saving statistics to a file
void save_stats_to_file() {
    if (json_messages == NULL || json_object_array_length(json_messages) == 0) {
        return;
    }

    char *file_extension = config.use_json ? ".json" : ".csv";
    unsigned int totalLength = strlen(config.stats_filepath) + strlen(file_extension) + 1; // +1 per il terminatore '\0'
    char *fullPath = (char *) malloc(totalLength);

    if (fullPath == NULL) {
        fprintf(stderr, "Errore di allocazione della memoria.\n");
        return;
    }

    snprintf(fullPath, totalLength, "%s%s", config.stats_filepath, file_extension);

    FILE *file = fopen(fullPath, "w");
    free(fullPath); // Libera la memoria dopo l'uso

    if (file == NULL) {
        fprintf(stderr, "Impossibile aprire il file per la scrittura.\n");
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
        for (int i = 0; i < array_len; i++) {
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
        printf("Saving statistics...\n");

        // Wait for the specified time before the next save
        sleep(config.save_interval_seconds);

        // Save statistics to a file
        save_stats_to_file();
    }
    return NULL;
}

int main() {
    // Load the configuration
    if (read_config("../config.yaml", &config) != 0) {
        fprintf(stderr, "Failed to read config.yaml\n");
        return 1;
    }

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
    release_config(&config);

    return 0;
}
