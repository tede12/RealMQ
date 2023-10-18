#include <zmq.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h> // for sleep()
#include "../common/zhelpers.h"
#include "../common/utils.h"
#include "../common/config.h"
#include "../common/qos.h"
#include "../common/logger.h"

Logger server_logger;

int main(void) {
    printf("Server started\n");

    int rc;
    void *context = create_context();

//    // DISH for SERVER
//    void *dish = create_socket(context, ZMQ_DISH, "udp://127.0.0.1:5555", 2000, "GRP");
//    // -----------------------------------------------------------------------------------------------------------------
//    // RADIO for RESPONDER
//    void *radio = create_socket(context, ZMQ_RADIO, "udp://127.0.0.1:5556", 2000, NULL);
//    // -----------------------------------------------------------------------------------------------------------------

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


    // Dish socket
    void *dish = create_socket(
            context, get_zmq_type(SERVER),
            get_address(MAIN_ADDRESS),
            config.signal_msg_timeout,
            get_group(MAIN_GROUP)
    );

    // Responder socket
    void *context2 = create_context();
    void *radio = create_socket(
            context2, ZMQ_RADIO,
            get_address(RESPONDER),
            config.signal_msg_timeout,
            NULL
    );


    int count_msg = 0;

    while (1) {
        char buffer[1024];
        rc = zmq_recv(dish, buffer, 1023, 0);
        if (rc == -1 && errno == EAGAIN) {
            // Timeout occurred
            continue;
        }

        // Check if prefix of the message is "HB" (heartbeat)
        if (strncmp(buffer, "HB", 2) == 0) {
            // Heartbeat received
            printf("Heartbeat received\n");
            // last id received
            char *last_id = get_message_id(-1);
            if (last_id == NULL) {
                logger(LOG_LEVEL_WARN, "No message ids found, message_ids count: %zu", num_message_ids);
                continue;
            }
            logger(LOG_LEVEL_WARN, "Sending last_id to client REP: %s", last_id);
//            zmq_send_group(radio, "REP", last_id, 0);
            process_message_ids(radio, last_id);
            continue;
        }

        // This is the case of a message received from a client
        buffer[rc] = '\0'; // Null-terminate the string
        printf("Received from client [MAIN]: %s\n", buffer);

        // Should be a double in string format
        add_message_id(buffer);

        count_msg++;

//        // Send a reply
//        const char *msg = "Message sent from server [RESPONDER]";
//        zmq_send_group(radio, "REP", msg, 0);
    }

    zmq_close(radio);
    zmq_close(dish);
    zmq_ctx_destroy(context);
    return 0;
}
