#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include "utils/utils.h"
#include "core/config.h"
#include "core/logger.h"
#include "utils/memory_leak_detector.h"
#include "core/zhelpers.h"
#include "qos/dynamic_array.h"
#include "qos/buffer_segments.h"

pthread_mutex_t g_count_msg_mutex = PTHREAD_MUTEX_INITIALIZER;
Logger server_logger;
long long received_messages = 0;
#define MAX(a, b) (((a)>(b))?(a):(b))


#define QOS_TEST


void send_ids(void *radio) {
    // Create a buffer with the IDs
    BufferSegmentArray segments_array = marshal_and_split(&g_array);

    // Send segments with max size of MAX_SEGMENT_SIZE
    for (size_t i = 0; i < segments_array.count; i++) {
        printf("Sending segment %s\n", segments_array.segments[i].data);

        // Send the buffer with the IDs
        zmq_send_group(
                radio,
                get_group(RESPONDER_GROUP),
                segments_array.segments[i].data,
                0
        );
    }

    // Send a wakeup message
    if (segments_array.count == 0) {
        // Send an empty message to notify the client that there are no more IDs (needed for cleaning the g_array)
        zmq_send_group(
                radio,
                get_group(RESPONDER_GROUP),
                "WAKEUP",
                0
        );
    }

    free_segment_array(&segments_array);

    // Clean the array of IDs
    clean_all_elements(&g_array);
}

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
        if (zmq_receive(dish, buffer, sizeof(buffer), 0) == -1) {
            continue;
        }

        // If message start with "STOP" then stop the server
        if (strncmp(buffer, "STOP", 4) == 0) {
            logger(LOG_LEVEL_INFO, "Received STOP signal");
            send_ids(radio);    // Notify last IDs
            break;
        } else if (strncmp(buffer, "START", 5) == 0) {
            // Needed for the first message for TCP (slow joiner syndrome)
            logger(LOG_LEVEL_INFO, "Received START signal");
            continue;
        } else if (strncmp(buffer, "HB", 2) == 0) {
#ifdef QOS_TEST
            // UDP Packet Detection
            send_ids(radio);
#endif
            continue;
        }

        Message *msg = unmarshal_message(buffer);
        if (msg == NULL) {
            continue;
        }

        // logger(LOG_LEVEL_DEBUG, "Received message, with ID: %lu", msg->id);


#ifdef QOS_TEST
        add_to_dynamic_array(&g_array, &msg->id);
#endif
        pthread_mutex_lock(&g_count_msg_mutex);
        count_msg++;
        // keep max from received messages and msg->id
        received_messages = MAX(received_messages, msg->id);
        pthread_mutex_unlock(&g_count_msg_mutex);

        release_element(msg, sizeof(Message));

        if (count_msg % 1000 == 0 && count_msg != 0) {
            logger(LOG_LEVEL_INFO, "Received %d messages", count_msg);
        }
    }

    // Wait a bit before sending the stop signal to the responder thread (in client)
    sleep(3);
    zmq_send_group(
            radio,
            get_group(RESPONDER_GROUP),
            "STOP",
            0
    );

    logger(LOG_LEVEL_INFO2, "Total received messages: %d", count_msg);
    logger(LOG_LEVEL_INFO2, "Max received message ID: %lld", received_messages);

    zmq_close(radio);
    zmq_close(dish);
    zmq_ctx_destroy(context);
    zmq_ctx_destroy(context2);

    release_dynamic_array(&g_array);
    release_config();


    check_for_leaks(); // Check for memory leaks

    return 0;
}
