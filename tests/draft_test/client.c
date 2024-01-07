#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h> // for sleep()
#include <pthread.h>
#include <ctype.h>
#include "string_manip.h"
#include "utils/utils.h"
#include "utils/time_utils.h"
#include "core/zhelpers.h"
#include "core/logger.h"
#include "core/config.h"
#include "qos/accrual_detector.h"
#include "qos/dynamic_array.h"


void *g_shared_context;
int g_count_msg = 0;
// mutex for g_count_msg
pthread_mutex_t g_count_msg_mutex = PTHREAD_MUTEX_INITIALIZER;
DynamicArray g_array;


Logger client_logger;

void *responder_thread(void *arg) {
    void *socket = (void *) arg;
    while (!interrupted) {
        char buffer[1024];
        if (zmq_receive(socket, buffer, 0) == -1) {
            continue;
        }

        // Retrieve all messages ids sent from the client to the server
        DynamicArray *new_array = unmarshal_uint64_array(buffer);
        if (new_array == NULL) {
            continue;
        }

        // Get the last message id from the array data.
        uint64_t *last_id = get_element_by_index(new_array, -1);
        uint64_t *first_id = get_element_by_index(&g_array, 0);

        size_t missed_count = 0;
        // todo: Fix it need to get from the new array not in the g_array

        // Remove from the low index to the last index of the array messages, if they are missing, count them.
        for (uint64_t i = *first_id; i <= *last_id; i++) {
            if (remove_element_by_id(&g_array, i, true) == -1) {
                logger(LOG_LEVEL_WARN, "Message with ID %lu is missing, with message: %s", i, g_array.data[i]);
                // todo should be resent the message (here)
                missed_count++;
            }
        }

        // Release the resources
        release_dynamic_array(new_array);
        free(new_array);


        update_phi_detector(missed_count);

        if (missed_count > 0)
            logger(LOG_LEVEL_WARN, "Missed count: %zu", missed_count);
    }

    logger(LOG_LEVEL_DEBUG, "Responder thread exiting");
    return NULL;
}


void client_thread(void *thread_id) {
    signal(SIGINT, handle_interrupt); // Register the interruption handling function

    int thread_num = *(int *) thread_id;

    int rc;

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

    int count_msg = 0;

    // Message Loop
    while (!interrupted) {

        // Only used for STOPPING thread
        if (count_msg == config.num_messages) {
            for (int i = 0; i < 3; i++) {
                // Send 3 messages to notify the server that the client has finished sending messages
                zmq_send_group(radio, "GRP", "STOP", 0);
                sleep(1);
                logger(LOG_LEVEL_INFO, "Sent STOP message");
                handle_interrupt(0);
            }
            break;
        }

        // ----------------------------------------- PACKET DETECTION --------------------------------------------------
        // Send a heartbeat before starting to send messages
        send_heartbeat(radio, get_group(MAIN_GROUP), false);
        // -------------------------------------------------------------------------------------------------------------

        // Create a message ID
        Message *msg = create_element("Hello World!");
        if (msg == NULL) {
            continue;
        }

        add_to_dynamic_array(&g_array, msg);
        const char *msg_buffer = marshal_message(msg);
        if (msg_buffer == NULL) {
            free((void *) msg_buffer);
            continue;
        }

        rc = zmq_send_group(radio, "GRP", msg_buffer, 0);
        free((void *) msg_buffer);

        if (rc == -1) {
            printf("Error in sending message\n");
            break;
        }
        logger(LOG_LEVEL_INFO2, "Sent message with ID: %lu", msg->id);

        if (count_msg % 100000 == 0 && count_msg != 0) {
            logger(LOG_LEVEL_INFO, "Sent %d messages with Thread %d", count_msg, thread_num);
        }
        count_msg++;
        release_element(msg, sizeof(Message));

        // Random sleep from 0 to 200ms
        rand_sleep(0, 50);
    }

    // Add count_msg to g_count_msg
    pthread_mutex_lock(&g_count_msg_mutex);
    g_count_msg += count_msg;
    pthread_mutex_unlock(&g_count_msg_mutex);

    // Release the resources
    zmq_close(radio);
    logger(LOG_LEVEL_DEBUG, "Thread %d finished", thread_num);
}

int main(void) {

    sleep(3);    // wait for server to start
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

    Logger_init("realmq_sever", &logger_config, &client_logger);

    if (read_config("../config.yaml") != 0) {
        logger(LOG_LEVEL_ERROR, "Failed to read config.yaml");
        return 1;
    }

    // Print configuration
    print_configuration();

    // Initialize the dynamic array
    init_dynamic_array(&g_array, 100000, sizeof(Message));

    void *dish = create_socket(
            g_shared_context,
            ZMQ_DISH,
            get_address(RESPONDER_ADDRESS),
            config.signal_msg_timeout,
            get_group(RESPONDER_GROUP)
    );

    pthread_t responder;
    pthread_create(&responder, NULL, responder_thread, dish);

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

    // Wait for the server thread to finish
    pthread_join(responder, NULL);

    zmq_close(dish);
    logger(LOG_LEVEL_INFO, "Closed DISH socket");
    zmq_ctx_destroy(g_shared_context);
    logger(LOG_LEVEL_INFO, "Destroyed context");

    logger(LOG_LEVEL_INFO, "Total messages sent: %d", g_count_msg);
    release_config();
    release_dynamic_array(&g_array);
    logger(LOG_LEVEL_INFO, "Released configuration");

    return 0;
}

