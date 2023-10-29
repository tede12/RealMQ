#include <zmq.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h> // for sleep()
#include "core/zhelpers.h"
#include "utils/utils.h"
#include "core/config.h"
#include "qos/accrual_detector.h"
#include "core/logger.h"

Logger server_logger;

int main(void) {
    printf("Server started\n");

    int rc;
    void *context = create_context();

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
    print_configuration();


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

    while (!interrupted) {
        char buffer[1024];
        rc = zmq_recv(dish, buffer, 1023, 0);
        if (rc == -1 && errno == EAGAIN) {
            // Timeout occurred
            continue;
        }

        // If message start with "STOP" then stop the server
        if (strncmp(buffer, "STOP", 4) == 0) {
            logger(LOG_LEVEL_INFO, "Received STOP signal");
            break;
        }

        buffer[rc] = '\0';
        logger(LOG_LEVEL_INFO2, "Received message: %s", buffer);


        count_msg++;

        if (count_msg % 100000 == 0 && count_msg != 0) {
            logger(LOG_LEVEL_INFO, "Received %d messages", count_msg);
        }
    }

    logger(LOG_LEVEL_INFO2, "Total received messages: %d", count_msg);

    zmq_close(radio);
    zmq_close(dish);
    zmq_ctx_destroy(context);
    zmq_ctx_destroy(context2);

    release_config();

    return 0;
}
