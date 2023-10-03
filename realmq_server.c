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
#include <stdbool.h>

#define ADDRESS "tcp://127.0.0.1:5555"
#define NUM_THREADS 100
#define NUM_MESSAGES 1000
#define DEADLINE_MS 2000 // Deadline in milliseconds
#define STATS_FILEPATH "../server_stats"
#define SAVE_INTERVAL_SECONDS 10
#define USE_JSON false

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
void process_json_message(const char *json_str, long long recv_time) {
    // "{ \"message\": \"Thread 20 - Message 400\", \"send_time\": 358913413 }"
    json_object *json_msg = json_tokener_parse(json_str);
    if (json_msg) {
        // Add the message to the statistical data

        // Create a JSON object for the message
        json_object *jobj = json_object_new_object();
        json_object_object_add(jobj, "id", json_object_object_get(json_msg, "id"));
//        json_object_object_add(jobj, "message", json_object_object_get(json_msg, "message"));
        json_object_object_add(jobj, "send_time", json_object_object_get(json_msg, "send_time"));
        json_object_object_add(jobj, "recv_time", json_object_new_int64(recv_time));
        json_object_array_add(json_messages, jobj);
    }
}


// Function executed by the server
void *server_thread(void *args) {
    signal(SIGINT, handle_interrupt); // Register the interruption handling function

    void *context = zmq_ctx_new();
    void *socket = zmq_socket(context, ZMQ_SUB);
    zmq_bind(socket, ADDRESS);
    zmq_setsockopt(socket, ZMQ_SUBSCRIBE, "", 0);

    int messages_received = 0; // Counter for received messages

    while (!interrupted) {
        char message[512];
        zmq_recv(socket, message, sizeof(message), 0);
        long long recv_time = get_current_time_nanos();

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
void save_stats_to_file(bool use_json) {
    if (json_messages == NULL || json_object_array_length(json_messages) == 0) {
        return;
    }

    if (use_json) {
        // Save to json

        json_object *jobj = json_object_new_object();
        // Add the array of messages to the JSON statistics object
        json_object_object_add(jobj, "messages", json_messages);

        const char *json_data = json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PRETTY);
        FILE *json_file = fopen(STATS_FILEPATH ".json", "w");
        if (json_file) {
            fprintf(json_file, "%s\n", json_data);
            fclose(json_file);
        }
        // Clear json_messages
        json_messages = json_object_new_array(); // Added to store all messages
    } else {
        // Save to csv
        FILE *csv_file = fopen(STATS_FILEPATH ".csv", "w");
        if (csv_file) {
            // Write the CSV header
//            fprintf(csv_file, "id,send_time,recv_time\n");
            fprintf(csv_file, "id,num,diff\n");


            unsigned long array_len = json_object_array_length(json_messages);
            for (int i = 0; i < array_len; i++) {
                json_object *json_msg = json_object_array_get_idx(json_messages, i);

                // id, num = i, diff
                long long diff = json_object_get_int64(json_object_object_get(json_msg, "recv_time")) -
                                 json_object_get_int64(json_object_object_get(json_msg, "send_time"));

                int new_diff = diff / 1000000;

                if (json_msg) {
                    fprintf(
                            csv_file, "%s,%d,%d\n",
                            json_object_get_string(json_object_object_get(json_msg, "id")),
                            i + 1,
                            new_diff
                    );


                    /*
                    fprintf(
                        csv_file, "%s,%s,%s\n",
                        json_object_get_string(json_object_object_get(json_msg, "id")),
                        json_object_get_string(json_object_object_get(json_msg, "send_time")),
                        json_object_get_string(json_object_object_get(json_msg, "recv_time"))

                    );
                     */
                }
            }

            fclose(csv_file);
        }
    }


}

// Function for handling periodic statistics saving
void *stats_saver_thread(void *args) {
    while (!interrupted) {
        printf("Saving statistics...\n");

        // Wait for the specified time before the next save
        sleep(SAVE_INTERVAL_SECONDS);

        // Save statistics to a file
        save_stats_to_file(USE_JSON);
    }
    return NULL;
}

int main() {
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
    save_stats_to_file(USE_JSON);

    // Deallocate json_messages before exiting
    json_object_put(json_messages);

    return 0;
}
