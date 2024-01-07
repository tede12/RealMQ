#include <zmq.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h> // for sleep()
#include "utils/utils.h"
#include "core/config.h"
#include "core/logger.h"
#include "core/zhelpers.h"
#include "qos/accrual_detector.h"
#include "qos/dynamic_array.h"

Logger server_logger;
DynamicArray g_array;

int main(void) {
    printf("Server started\n");

    // Create a new dynamic array
    init_dynamic_array(&g_array, 100000, sizeof(uint64_t));

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

    get_address(MAIN_ADDRESS);
    get_address(RESPONDER_ADDRESS);

    void *context = create_context();

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
            get_address(RESPONDER_ADDRESS),
            config.signal_msg_timeout,
            NULL
    );

    int count_msg = 0;

    while (!interrupted) {
        char buffer[1024];
        if (zmq_receive(dish, buffer, 0) == -1) {
            continue;
        }

        // If message start with "STOP" then stop the server
        if (strncmp(buffer, "STOP", 4) == 0) {
            logger(LOG_LEVEL_INFO, "Received STOP signal");
            break;
        } else if (strncmp(buffer, "START", 5) == 0) {
            // Needed for the first message for TCP (slow joiner syndrome)
            logger(LOG_LEVEL_INFO, "Received START signal");
            continue;
        } else if (strncmp(buffer, "HB", 2) == 0) {
            // UDP Packet Detection
//            logger(LOG_LEVEL_INFO2, "Received HB signal");

            // Create a buffer with the IDs
            const char *ids_buffer = marshal_uint64_array(&g_array);

            // Clean the array of IDs
            clean_all_elements(&g_array);

            // Send the buffer with the IDs
            zmq_send_group(radio, get_group(RESPONDER_GROUP), ids_buffer, 0);
//            logger(LOG_LEVEL_INFO2, "Sent IDS Buffer");
            free((void *) ids_buffer);
            continue;
        }

        Message *msg = unmarshal_message(buffer);
        if (msg == NULL) {
            continue;
        }

        add_to_dynamic_array(&g_array, &msg->id);

//        logger(LOG_LEVEL_INFO2, "Received message, with ID: %lu, Content: %s", msg->id, msg->content);

        count_msg++;
        release_element(msg, sizeof(Message));

        if (count_msg % 100000 == 0 && count_msg != 0) {
            logger(LOG_LEVEL_INFO, "Received %d messages", count_msg);
        }
    }

    logger(LOG_LEVEL_INFO2, "Total received messages: %d", count_msg);

    zmq_close(radio);
    zmq_close(dish);
    zmq_ctx_destroy(context);
    zmq_ctx_destroy(context2);

    release_dynamic_array(&g_array);
    release_config();


    return 0;
}
