#include <string.h>
#include <pthread.h>
#include <zmq.h>
#include <time.h>
#include <unistd.h> // for sleep function
#include <signal.h>
#include <json-c/json.h>
#include "utils/utils.h"
#include "core/config.h"
#include "core/zhelpers.h"
#include "core/logger.h"
#include "utils/time_utils.h"
#include "utils/fs_utils.h"


// ---------------------------------------- Global configuration -------------------------------------------------------
Logger server_logger;

// Variables for periodic statistics saving
json_object *json_messages = NULL; // Added to store all messages
// ---------------------------------------------------------------------------------------------------------------------

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
    void *context2 = create_context();
    void *responder = create_socket(
            context2, ZMQ_RADIO,
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

#ifdef QOS_RETRANSMISSION
        // Check if prefix of the message is "HB" (heartbeat)
        if (strncmp(message, "HB", 2) == 0) {
            // Heartbeat received
            logger(LOG_LEVEL_INFO, "Heartbeat received");

            // last id received
            char *last_id = get_message_id(-1);
            process_message_ids(responder, last_id);
            continue;
        }
#endif

        // Process the received message
        handle_message(message);

        // Process the received JSON message and update statistics
        process_json_message(message, recv_time);

        // Increment the received messages counter
        messages_received++;

        // Extract the ID from the message
        json_object *json_msg = json_tokener_parse(message);
        if (json_msg) {
            // This part send with the socket RESPONDER back the ID of the message to the client
            json_object *id_obj;
            if (json_object_object_get_ex(json_msg, "id", &id_obj)) {
                const char *id_str = json_object_get_string(id_obj);

#ifdef QOS_RETRANSMISSION
                // Store the ID
                add_message_id(id_str);
#endif

#ifndef REALMQ_VERSION
                // Send the ID back to the client
                int rc = zmq_send(receiver, id_str, strlen(id_str), 0);
                if (rc == -1) {
                    logger(LOG_LEVEL_ERROR, "Error in sending message");
                }
#endif
            }


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


// Function for handling periodic statistics saving
void *stats_saver_thread(void *args) {
    while (!interrupted) {
        // Wait for the specified time before the next save
        sleep(config.save_interval_seconds);

        // Save statistics to a file
        save_stats_to_file(json_messages);
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

    Logger_init("realmq_sever", &logger_config, &server_logger);


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
    save_stats_to_file(json_messages);

    // Deallocate json_messages before exiting
    json_object_put(json_messages);

    // Release the configuration
    release_config();
    release_logger();

    return 0;
}
