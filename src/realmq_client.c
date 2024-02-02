#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#include "utils/utils.h"
#include "utils/time_utils.h"
#include "core/zhelpers.h"
#include "core/logger.h"
#include "core/config.h"
#include "qos/accrual_detector.h"
#include "qos/dynamic_array.h"
// #include "utils/memory_leak_detector.h"
#include "qos/accrual_detector/phi_accrual_failure_detector.h"
#include "string_manip.h"

#define QOS_ENABLE
//#define TCP_ONLY

// ============================================= Global configuration ==================================================
void *g_shared_context;
void *g_radio;
int g_count_msg = 0;
int g_missed_count = 0;

Logger client_logger;

// Mutex for g_count_msg
pthread_mutex_t g_count_msg_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_array_mutex = PTHREAD_MUTEX_INITIALIZER;
// =====================================================================================================================

#ifdef QOS_ENABLE
void *responder_thread(void *arg) {
    void *socket = (void *) arg;
    while (true) {
        char buffer[2048];
        if (zmq_receive(socket, buffer, sizeof(buffer), 0) == -1) {
            continue;
        }

        if (strcmp(buffer, "STOP") == 0) {
            logger(LOG_LEVEL_INFO, "Received STOP message");
            break;
        }

        // printf("Buffer: %s\n", buffer);

        // Retrieve all messages ids sent from the client to the server
        DynamicArray *new_array = unmarshal_uint64_array(buffer);
        if (new_array == NULL) {
            continue;
        }

        pthread_mutex_lock(&g_array_mutex);
        int missed_count = diff_from_arrays(&g_array, new_array, g_radio);
        pthread_mutex_unlock(&g_array_mutex);

        // Release the resources
        release_dynamic_array(new_array);
        free(new_array);

        // Update failure detector based on missed_count
        if (missed_count) logger(LOG_LEVEL_WARN, "Missed count: %d", missed_count);
        adjust_intervals(g_detector->state->history, missed_count);

        // Update the g_missed_count
        if (missed_count > 0) {
            pthread_mutex_lock(&g_count_msg_mutex);
            g_missed_count += missed_count;
            pthread_mutex_unlock(&g_count_msg_mutex);
        }
    }

    logger(LOG_LEVEL_DEBUG, "Responder thread exiting");
    return NULL;
}
#endif


void client_thread(void *thread_id) {
    signal(SIGINT, handle_interrupt); // Register the interruption handling function

    int thread_num = *(int *) thread_id;
    int rc;

    // Unique radio for each thread
    void *radio = create_socket(
            g_shared_context,
            get_zmq_type(CLIENT),
            get_address(MAIN_ADDRESS),
            config.signal_msg_timeout,
            NULL
    );

    // Send first message only with TCP for avoiding the problem of "slow joiner syndrome."
    if (get_protocol_type() == TCP) {
        rc = zmq_send_group(radio, "GRP", "START", 0);
        if (rc == -1) {
            printf("Error in sending message\n");
            return;
        }
        logger(LOG_LEVEL_INFO, "Sent START message");
        sleep(2);
    }

    // Wait for the specified time before starting to send messages
    s_sleep(config.client_action->sleep_starting_time);

    int count_msg = 0;

#ifdef QOS_ENABLE
    // Send the first heartbeat
    send_heartbeat(NULL, NULL, true);
#endif

    // Message Loop
    while (!interrupted) {

        // Only used for STOPPING thread
        if (count_msg == config.num_messages) {

#ifdef QOS_ENABLE
            // Before sending the STOP message, wait until the g_array is empty (all messages are sent)
            while (true) {
                // Wait until g_array is empty
                pthread_mutex_lock(&g_array_mutex);
                if (g_array.size == 0) {
                    pthread_mutex_unlock(&g_array_mutex);
                    break;
                }
                // Send a heartbeat message (for flushing the messages)
                send_heartbeat(radio, get_group(MAIN_GROUP), true);

                // logger(LOG_LEVEL_INFO, "[*stop*] Waiting for g_array to be empty (size: %d)", g_array.size);

                pthread_mutex_unlock(&g_array_mutex);
                sleep(1);
            }
#endif
            // Send STOP message
            for (int i = 0; i < 3; i++) {
                // Send 3 messages to notify the server that the client has finished sending messages
#ifndef TCP_ONLY
                zmq_send_group(radio, "GRP", "STOP", 0);
#else
                zmq_send(radio, "STOP", 4, 0);
#endif
                sleep(1);
                logger(LOG_LEVEL_INFO, "Sent STOP message");
                handle_interrupt(0);
            }
            break;
        }

#ifdef QOS_ENABLE
        // ----------------------------------------- Send Message ------------------------------------------------------
        // Before sending a message, I check if the responder already finished sending ACKs for the previous messages
        // If not, I wait until the responder finishes sending ACKs, this prevents for sending twice the same message
        // Because It remains in the g_array until the responder finishes sending ACKs for more than a row

        int try_lock_result = pthread_mutex_trylock(&g_array_mutex);
        if (try_lock_result == EBUSY) {
            // The mutex was locked by the responder thread
            // logger(LOG_LEVEL_WARN, "[client] Waiting for g_array to be empty (size: %d)", g_array.size);
            continue;

        } else if (try_lock_result == 0) {
            // The mutex was not locked, and now it's locked by this thread
            // Since we only wanted to check, immediately unlock it
            pthread_mutex_unlock(&g_array_mutex);
            // logger(LOG_LEVEL_DEBUG, "Client thread is unlocking g_array_mutex");
        } else {
            logger(LOG_LEVEL_ERROR, "Error in pthread_mutex_trylock");
            continue;
        }

        // ----------------------------------------- PACKET DETECTION --------------------------------------------------
        // Send a heartbeat before starting to send messages
        send_heartbeat(radio, get_group(MAIN_GROUP), false);
        // -------------------------------------------------------------------------------------------------------------
#endif
        pthread_mutex_lock(&g_array_mutex);

        // Create a message of total size = config.message_size
        char message[config.message_size];
        sprintf(message, "Thread %d - Message %d - ", thread_num, count_msg);

        unsigned long current_len = strlen(message);
        char *rnd_string = random_string(config.message_size - current_len);
        strcat(message, rnd_string);

        Message *msg = create_element(message);
        // printf("Message: %s\n", message);
        free(rnd_string);
        if (msg == NULL) {
            continue;
        }

#ifdef QOS_ENABLE
        add_to_dynamic_array(&g_array, msg);
#endif
        // logger(LOG_LEVEL_DEBUG, "Sending message with ID: %" PRIu64, msg->id);

        // ----------------------------------------- Send message to server --------------------------------------------
        const char *msg_buffer = marshal_message(msg);

        if (msg_buffer == NULL) {
            release_element(msg, sizeof(Message));
            pthread_mutex_unlock(&g_array_mutex);
            continue;
        }

#ifdef TCP_ONLY
        rc = zmq_send(radio, msg_buffer, strlen(msg_buffer), 0);

        // Check if the message was sent correctly
        zmq_recv(radio, (void *) msg_buffer, sizeof(msg_buffer), 0);
//        zmq_receive(radio, message, sizeof(message), 0);
#else
        rc = zmq_send_group(radio, "GRP", msg_buffer, 0);
#endif

        // Free msg_buffer after It's been used
        free((void *) msg_buffer);

        if (rc == -1) {
            printf("Error in sending message\n");
            release_element(msg, sizeof(Message));
            pthread_mutex_unlock(&g_array_mutex);
            break;
        }

        // -------------------------------------------------------------------------------------------------------------

        if (count_msg % 1000 == 0 && count_msg != 0) {
            logger(LOG_LEVEL_INFO, "Sent %d messages with Thread %d", count_msg, thread_num);
        }
        count_msg++;

        release_element(msg, sizeof(Message));


        pthread_mutex_unlock(&g_array_mutex);

#ifndef QOS_ENABLE
        try_reconnect(g_shared_context, &radio, get_address(MAIN_ADDRESS), get_zmq_type(CLIENT));
#endif

        // Random sleep from 0 to 10ms
        rand_sleep(0, 1);
    }

// Add count_msg to g_count_msg
    pthread_mutex_lock(&g_count_msg_mutex);
    g_count_msg +=
            count_msg;
    pthread_mutex_unlock(&g_count_msg_mutex);

// Release the resources
    zmq_close(radio);
    logger(LOG_LEVEL_DEBUG,
           "***Exiting client thread %d.", thread_num);
}


int main(void) {
    sleep(3);   // Wait for the server to start

    printf("Client started\n");

    // Create a single context for the entire application
    g_shared_context = create_context();

    // Load the configuration
    logConfig logger_config = {
            .show_timestamp = 1,
            .show_logger_name = 1,
            .show_thread_id = 1,
            .log_to_console = 1,
            .log_level = LOG_LEVEL_INFO
    };

    Logger_init("realmq_client", &logger_config, &client_logger);

    if (read_config("../config.yaml") != 0) {
        logger(LOG_LEVEL_ERROR, "Failed to read config.yaml");
        return 1;
    }

    // Print configuration
    print_configuration();

    // Initialize the dynamic array
    init_dynamic_array(&g_array, 100000, sizeof(Message));

#ifdef QOS_ENABLE
    // Load the configuration for the failure detector
    phi_accrual_detector detector_config = {
            .threshold = 6,
            .max_sample_size = 1000,
            .min_std_deviation_ms = 10.0f,
            .acceptable_heartbeat_pause_ms = 0.0f,
            .first_heartbeat_estimate_ms = 10.0f,
            .state = NULL
    };

    // Initialize the failure detector
    init_phi_accrual_detector(&detector_config);


    void *dish = create_socket(
            g_shared_context,
            ZMQ_DISH,
            get_address(RESPONDER_ADDRESS),
            config.signal_msg_timeout,
            get_group(RESPONDER_GROUP)
    );



    // Used only for sending missed messages
    g_radio = create_socket(
            g_shared_context,
            get_zmq_type(CLIENT),
            get_address(MAIN_ADDRESS),
            config.signal_msg_timeout,
            NULL
    );
#endif

    timespec start_time = get_current_time();
    logger(LOG_LEVEL_DEBUG, "Start Time: %.3f", get_current_time_value(&start_time));

    // ============================================= Threads Part ======================================================
#ifdef QOS_ENABLE
    pthread_t responder;
    pthread_create(&responder, NULL, responder_thread, dish);
#endif

    // Use threads to send messages
    pthread_t clients[config.num_threads];
    int thread_ids[config.num_threads];
    for (int i = 0; i < config.num_threads; i++) {
        thread_ids[i] = i;
        pthread_create(&clients[i], NULL, (void *) client_thread, &thread_ids[i]);
    }

    // Wait for client threads to finish
    for (int i = 0; i < config.num_threads; i++) {
        pthread_join(clients[i], NULL);
    }

#ifdef QOS_ENABLE
    // Wait for the server thread to finish
    pthread_join(responder, NULL);
#endif
    // ============================================= End Threads Part ==================================================

    logger(LOG_LEVEL_DEBUG, "Execution Time: %.3f ms (+ %d ms of sleep starting time)",
           get_elapsed_time(start_time, NULL), config.server_action->sleep_starting_time);

#ifdef QOS_ENABLE
    zmq_close(dish);
    zmq_close(g_radio);
#endif
    logger(LOG_LEVEL_INFO, "Closed DISH socket");
    zmq_ctx_destroy(g_shared_context);
    logger(LOG_LEVEL_INFO, "Destroyed context");

    logger(LOG_LEVEL_INFO2, "Total messages sent: %d", g_count_msg);
    logger(LOG_LEVEL_INFO2, "Total messages missed: %d", g_missed_count);

    // Release the resources
    release_config();
    release_dynamic_array(&g_array);
#ifdef QOS_ENABLE
    delete_phi_accrual_detector(g_detector);
#endif
    logger(LOG_LEVEL_INFO, "Released configuration");

//    check_for_leaks();  // Check for memory leaks

    return 0;
}




