#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h> // for sleep()
#include <pthread.h>
#include "core/zhelpers.h"
#include "utils/utils.h"
#include "core/logger.h"
#include "core/config.h"
#include "string_manip.h"

void *g_shared_context;
int g_count_msg = 0;
// mutex for g_count_msg
pthread_mutex_t g_count_msg_mutex = PTHREAD_MUTEX_INITIALIZER;

Logger client_logger;

void *responder_thread(void *arg) {
    void *socket = (void *) arg;
    while (!interrupted) {
        char buffer[1024];
        int rc = zmq_recv(socket, buffer, 1023, 0);
        if (rc == -1 && errno == EAGAIN) {
            // Timeout occurred
            continue;
        }

        buffer[rc] = '\0'; // Null-terminate the string

        size_t missed_count = 0;
        char **missed_ids = process_missed_message_ids(buffer, &missed_count);
        logger(LOG_LEVEL_WARN, "Missed count: %zu", missed_count);

    }

    logger(LOG_LEVEL_DEBUG, "Responder thread exiting");
    return NULL;
}


void *client_thread(void *thread_id) {
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
            return NULL;
        }
        logger(LOG_LEVEL_INFO, "Sent START message");
        sleep(2);
    }

    char *msg_id;

    int count_msg = 0;

    while (!interrupted) {
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

        // Create a message ID
        msg_id = generate_uuid();

        rc = zmq_send_group(radio, "GRP", msg_id, 0);
        if (rc == -1) {
            printf("Error in sending message\n");
            break;
        }
        logger(LOG_LEVEL_INFO2, "Sent message with ID: %s", msg_id);

        if (count_msg % 100000 == 0 && count_msg != 0) {
            logger(LOG_LEVEL_INFO, "Sent %d messages with Thread %d", count_msg, thread_num);
        }
        count_msg++;
        free(msg_id);
    }

    // Add count_msg to g_count_msg
    pthread_mutex_lock(&g_count_msg_mutex);
    g_count_msg += count_msg;
    pthread_mutex_unlock(&g_count_msg_mutex);

    // Release the resources
    zmq_close(radio);
    logger(LOG_LEVEL_DEBUG, "Thread %d finished", thread_num);

    return NULL;
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
        pthread_create(&clients[i], NULL, client_thread, &thread_ids[i]);
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
    logger(LOG_LEVEL_INFO, "Released configuration");

    return 0;
}

