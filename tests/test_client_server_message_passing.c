#include "unity.h"
#include "utils/utils.h"
#include "qos/dynamic_array.h"
#include "qos/buffer_segments.h"
#include "utils/memory_leak_detector.h"
#include "core/zhelpers.h"
#include "core/config.h"
#include "core/logger.h"

void *g_shared_context;
Logger common_logger;
void *dish;
void *radio;

DynamicArray client_array;
DynamicArray server_array;

#define NUM_MESSAGES 3


void setUp(void) {
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

    Logger_init("realmq", &logger_config, &common_logger);

    if (read_config("../config.yaml") != 0) {
        logger(LOG_LEVEL_ERROR, "Failed to read config.yaml");
        exit(1);
    }

    init_dynamic_array(&client_array, 100, sizeof(Message));
    init_dynamic_array(&server_array, 100, sizeof(Message));

    // Client socket
    radio = create_socket(
            g_shared_context,
            get_zmq_type(CLIENT),
            get_address(MAIN_ADDRESS),
            config.signal_msg_timeout,
            NULL
    );

    // Server socket
    dish = create_socket(
            g_shared_context, get_zmq_type(SERVER),
            get_address(MAIN_ADDRESS),
            config.signal_msg_timeout,
            get_group(MAIN_GROUP)
    );
}

void tearDown(void) {
    // Release the resources
    zmq_close(radio);
    zmq_close(dish);
    zmq_ctx_destroy(g_shared_context);
    release_dynamic_array(&client_array);
    release_dynamic_array(&server_array);
    release_config();
}


void *server_thread(void *arg) {
    void *socket = (void *) arg;
    while (!interrupted) {
        char buffer[2048];
        // In server_thread function
        if (zmq_receive(socket, buffer, sizeof(buffer), 0) == -1) {
            continue;
        }

        if (strncmp(buffer, "STOP", 4) == 0) {
            logger(LOG_LEVEL_INFO, "Received STOP signal");
            break;
        }

        printf("[SERVER]: Buffer: %s\n", buffer);

        Message *msg = unmarshal_message(buffer);
        if (msg == NULL) {
            continue;
        }

        add_to_dynamic_array(&server_array, &msg->id);
        release_element(msg, sizeof(Message));
    }
    return NULL;
}

void *client_thread(void *arg) {
    int rc;
    size_t count = 0;
    // Message Loop
    while (count <= NUM_MESSAGES) {
        // Create a message ID
        Message *msg = create_element("Hello World!");
        if (msg == NULL) {
            continue;
        }

        add_to_dynamic_array(&client_array, msg);

        const char *msg_buffer = marshal_message(msg);
        if (msg_buffer == NULL) {
            free((void *) msg_buffer);
            continue;
        }

        rc = zmq_send_group(radio, "GRP", msg_buffer, 0);
        printf("[CLIENT]: Buffer: %s\n", msg_buffer);
        free((void *) msg_buffer);

        if (rc == -1) {
            printf("Error in sending message\n");
            break;
        }
        count++;
    }

    // Say to server that needs to shut down
    for (int i = 0; i < 3; i++)
        zmq_send_group(radio, "GRP", "STOP", 0);

    return NULL;
}


void start_client_server(void) {
    // Create thread for client
    pthread_t client;
    pthread_create(&client, NULL, client_thread, radio);


    // Create thread for server
    pthread_t server;
    pthread_create(&server, NULL, server_thread, dish);

    // Wait for the server and client thread to finish
    pthread_join(server, NULL);
    pthread_join(client, NULL);

    // Check if client and sever has same number of messages
    TEST_ASSERT_EQUAL_INT(server_array.size, client_array.size);

    // We can't go through
    assert(server_array.size == client_array.size);

    // Check if they got same message
    for (size_t i = 0; i < server_array.size; i++) {
        // Check the ID
        TEST_ASSERT_EQUAL_INT(
                ((Message *) server_array.data[i])->id,
                ((Message *) server_array.data[i])->id
        );

        // Check the content
        TEST_ASSERT_EQUAL_STRING(
                ((Message *) server_array.data[i])->content,
                ((Message *) server_array.data[i])->content
        );
    }
}

// The main function for running the tests
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(start_client_server);
    UNITY_END();

    check_for_leaks();  // Check for memory leaks
    return 0;
}
