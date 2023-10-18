#include <zmq.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h> // for sleep()
#include <pthread.h>
#include "../common/zhelpers.h"
#include "../common/utils.h"
#include "../common/qos.h"
#include "../common/logger.h"
#include "../common/config.h"


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
//        logger(LOG_LEVEL_WARN, "Message received from server [RESPONDER]: %s", buffer);
        size_t missed_count = 0;
        char **missed_ids = process_missed_message_ids(buffer, &missed_count);
        logger(LOG_LEVEL_WARN, "Missed count: %zu", missed_count);
//        printf("Message received from server [RESPONDER]: %s\n", buffer);
//        printf("Message received from server [RESPONDER] with ids\n");

    }
    return NULL;
}


void *client_thread(void *thread_id) {
    signal(SIGINT, handle_interrupt); // Register the interruption handling function


    int rc;

    void *context = create_context();
    void *radio = create_socket(
            context,
            get_zmq_type(CLIENT),
            get_address(MAIN_ADDRESS),
            config.signal_msg_timeout,
            NULL
    );

    timespec send_time = getCurrentTime();


    while (!interrupted) {
        // Create a message ID
        double recv_time = getCurrentTimeValue(NULL);
        char *msg_id = (char *) malloc(20 * sizeof(char));
        sprintf(msg_id, "%f", recv_time);

        // QoS - Send a heartbeat
        send_heartbeat(radio, "GRP", false);

        rc = zmq_send_group(radio, "GRP", msg_id, 0);
        if (rc == -1) {
            printf("Error in sending message\n");
            break;
        }

        // Save the message ID
        add_message_id(msg_id);

//        printf("Sent from client [MAIN]: %s (time spent since last message: %f)\n", msg_id,
//               getElapsedTime(send_time, NULL));

        send_time = getCurrentTime();


        sleep(0.2); // Send a message every second
    }

    zmq_close(radio);
    zmq_ctx_destroy(context);
}

int main(void) {

    sleep(3);    // wait for server to start
    printf("Client started\n");

    void *context_2 = create_context();
    int rc;

//    // RADIO for CLIENT
//    void *radio = create_socket(context, ZMQ_RADIO, "udp://127.0.0.1:5555", 1000, NULL);
//    // -----------------------------------------------------------------------------------------------------------------
//    // DISH for RESPONDER
//    // -----------------------------------------------------------------------------------------------------------------
//    void *dish = create_socket(context_2, ZMQ_DISH, "udp://127.0.0.1:5556", 1000, "REP");
//    // -----------------------------------------------------------------------------------------------------------------


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


    void *dish = create_socket(
            context_2,
            ZMQ_DISH,
            get_address(RESPONDER),
            config.signal_msg_timeout,
            get_group(RESPONDER_GROUP)
    );


    pthread_t responder;
    pthread_create(&responder, NULL, responder_thread, dish);


    // use a thread for handling the received messages

    pthread_t receiver;
    size_t thread_id = 1;
    pthread_create(&receiver, NULL, client_thread, &thread_id);

    // Wait for the server thread to finish
    pthread_join(responder, NULL);

    zmq_close(dish);
    zmq_ctx_destroy(context_2);

    return 0;
}
