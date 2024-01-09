#include "unity.h"
#include "utils/utils.h"
#include "string_manip.h"
#include "qos/dynamic_array.h"
#include "qos/buffer_segments.h"
#include "utils/memory_leak_detector.h"

char **generate_uuids(size_t count) {
    char **uuids = (char **) malloc(count * sizeof(char *));
    for (size_t i = 0; i < count; ++i) {
        uuids[i] = generate_uuid();
    }
    return uuids;
}

// These should be defined in common/utils/utils.c
//char **message_ids = NULL;
//size_t num_message_ids = 0;
//pthread_mutex_t message_ids_mutex;

void setUp(void) {
    // Initialize mutex
    pthread_mutex_init(&message_ids_mutex, NULL);

    // Generate message IDs
    num_message_ids = 5; // for instance
    message_ids = generate_uuids(num_message_ids);
}

void tearDown(void) {
    // Free the message IDs
    for (size_t i = 0; i < num_message_ids; ++i) {
        free(message_ids[i]);
    }
    free(message_ids);

    // Destroy the mutex
    pthread_mutex_destroy(&message_ids_mutex);
}

void test_process_missed_message_ids(void) {
    // Create a buffer of IDs, deliberately missing the last generated UUID
    char buffer[37 * 4 + 1];

    // missing message_ids[2]
    sprintf(buffer, "%s,%s,%s,%s", message_ids[0], message_ids[1], message_ids[3], message_ids[4]);

    // copy message_ids[2] to a tmp variable
    char *tmp = strdup(message_ids[2]);

    size_t missed_count = 0;

    // Call the function to test
    char **missed_ids = process_missed_message_ids(buffer, &missed_count);

    // Validate the results
    TEST_ASSERT_NOT_NULL(missed_ids);
    TEST_ASSERT_EQUAL(1, missed_count);

    // We expect the last UUID to be missed since it wasn't in the buffer (the only one missing)
    TEST_ASSERT_EQUAL_STRING(tmp, missed_ids[0]);

    // Clean up
    for (size_t i = 0; i < missed_count; ++i) {
        free(missed_ids[i]);
    }
    free(missed_ids);
    free(tmp);
}

void test_buffer_and_get_missed_ids(void) {
    /*
     * This test is doing what is done in the function `diff_from_arrays` in common/qos/dynamic_array.c
     */

    // Initialize the dynamic array
    init_dynamic_array(&g_array, 100, sizeof(Message));

    // --------------------------------------------- Client part -------------------------------------------------------

    // These should be sent messages (so 10 messages in total)
    for (int i = 1; i < 11; i++) {
        // "Hello World! %d", i
        char *custom_message = (char *) malloc(20 * sizeof(char));
        sprintf(custom_message, "Hello World! %d", i - 1);
        Message *msg = create_element(custom_message);
        if (msg == NULL) {
            continue;
        }
        add_to_dynamic_array(&g_array, msg);
        free(custom_message);
    }

    // --------------------------------------------- Responder part ----------------------------------------------------

    // These should be the arrived messages (so only 5 messages in total)
    char *buffer = "1|4|5|7|10";

    // Retrieve all messages ids sent from the client to the server
    DynamicArray *new_array = unmarshal_uint64_array(buffer);
    if (new_array == NULL) {
        return;
    }

    // Get the last message id from the array data.
    uint64_t *last_id = get_element_by_index(new_array, -1);
    uint64_t *first_id = get_element_by_index(&g_array, 0);

    int missed_count = 0;

    TEST_ASSERT_NOT_NULL(first_id);
    TEST_ASSERT_NOT_NULL(last_id);

    // Remove from the low index to the last index of the array messages, if they are missing, count them.
    size_t idx = 0;
    size_t idx_start = *first_id;
    size_t idx_end = *last_id;
    for (size_t i = idx_start; i <= idx_end; i++) {
        if (remove_element_by_id(new_array, i, true) == -1) {
            printf("Message with ID %zu is missing, with message: %s\n",
                   idx,
                   ((Message *) g_array.data[idx])->content);
            missed_count++;
        }
        idx++;
    }

    TEST_ASSERT_EQUAL_INT(5, missed_count);

    // Release the resources
    release_dynamic_array(new_array);
    release_dynamic_array(&g_array);
    free(new_array);
}

void test_client_server_missing_ids(void) {
    /*
     * This test is doing what is done in the function `diff_from_arrays` in common/qos/dynamic_array.c
     */

    // Initialize the dynamic array
    init_dynamic_array(&g_array, 100, sizeof(Message));

    // --------------------------------------------- Client part -------------------------------------------------------

    // Messages sent
    uint64_t values[] = {
            13, 14, 15, 16, 17, 18, 19,
            20, 21, 22, 23, 24, 25, 26
    };
    size_t values_size = sizeof(values) / sizeof(values[0]);

    for (int i = 0; i < values_size; i++) {
        char *custom_message = (char *) malloc(20 * sizeof(char));
        sprintf(custom_message, "Hello World! %llu", values[i]);
        Message *msg = create_element(custom_message);
        if (msg == NULL) continue;
        add_to_dynamic_array(&g_array, msg);
        free(custom_message);
    }

    // --------------------------------------------- Responder part ----------------------------------------------------
    // "13|14|15|16|17|21|22|23|24|25"

    // Messages received (from buffer)
    char *buffer = "13|14|15|16|17|21|22|24|25";

    // Retrieve all messages ids sent from the client to the server
    DynamicArray *new_array = unmarshal_uint64_array(buffer);
    if (new_array == NULL) {
        return;
    }

    int missed_count = diff_from_arrays(&g_array, new_array);
    // printing the missed ID should be 5, 6, 7, 10 (index) that corresponds to 18, 19, 20, 23 (values)

    TEST_ASSERT_EQUAL_INT(missed_count, 4);

    // Release the resources
    release_dynamic_array(new_array);
    release_dynamic_array(&g_array);
    free(new_array);
}

// The main function for running the tests
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_process_missed_message_ids);
    RUN_TEST(test_buffer_and_get_missed_ids);
    RUN_TEST(test_client_server_missing_ids);
    UNITY_END();

    check_for_leaks();  // Check for memory leaks
    return 0;
}
