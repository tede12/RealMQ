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

    // Reset the atomic message ID for each test
    reset_message_id();
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
        if (remove_element_by_id(new_array, i, true, true) == -1) {
            // printf("Message with ID %zu is missing, with message: %s\n",
            //        idx,
            //        ((Message *) g_array.data[idx])->content);
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


void print_array(DynamicArray *array, bool print_content) {
    printf("\n");
    for (size_t i = 0; i < array->size; ++i) {
        if (print_content) {
            printf("array[%zu]: %llu (%s)\n", i, *(uint64_t *) array->data[i], ((Message *) array->data[i])->content);
        } else {
            printf("array[%zu]: %llu\n", i, *(uint64_t *) array->data[i]);
        }
    }

    for (size_t i = 0; i < 50; ++i)
        printf("-");
    printf("\n");
}

void test_client_server_missing_ids(void) {
    /*
     * This test is doing what is done in the function `diff_from_arrays` in common/qos/dynamic_array.c
     */

    // Initialize the dynamic array
    init_dynamic_array(&g_array, 100, sizeof(Message));

    // --------------------------------------------- Client part -------------------------------------------------------

    int starting_value = 11;
    int ending_value = 23;

    // Messages sent
    for (int i = starting_value; i < ending_value; i++) {
        char *custom_message = (char *) malloc(20 * sizeof(char));
        sprintf(custom_message, "Hello World! %d", i);
        set_message_id(i);  // Only for testing purposes (to avoid generating random IDs)
        Message *msg = create_element(custom_message);
        if (msg == NULL) continue;
        // fake timeout or will fail next tests
        msg->timestamp = msg->timestamp - 6000; // make it old
        add_to_dynamic_array(&g_array, msg);
        free(custom_message);
    }


    char *buffer = marshal_uint64_array(&g_array);
    // printf("\n The BUFFER: %s\n", buffer);

    // --------------------------------------------- Responder part ----------------------------------------------------
    // Messages received (from buffer)
    char *buffer2 = "13|14|16|17|18|22|23";
    // printf("\n The BUFFER2: %s\n", buffer2);

    // Retrieve all messages ids sent from the client to the server
    DynamicArray *new_array = unmarshal_uint64_array(buffer2);
    if (new_array == NULL) {
        return;
    }

    // print 2 arrays
    // printf("\n# Messages sent: %zu\n", g_array.size);
    // print_array(&g_array, true);
    //
    // printf("\n# Messages received: %zu\n", new_array->size);
    // print_array(new_array, false);

    int missed_count = diff_from_arrays(&g_array, new_array, NULL);

    // printf("\n# Missed count: %d\n", missed_count);
    // print_array(&g_array, true);
    // printf("\n# New array size: %zu\n", new_array->size);
    // print_array(new_array, true);


    TEST_ASSERT_EQUAL_INT(missed_count, 5);

    // Check how many messages are in the array
    TEST_ASSERT_EQUAL_INT(g_array.size, 5);

    // Release the resources
    release_dynamic_array(new_array);
    release_dynamic_array(&g_array);
    free(new_array);
}

void test_big_differences(void) {
    /*
     * This test is doing what is done in the function `diff_from_arrays` in common/qos/dynamic_array.c
     */

    // Initialize the dynamic array
    init_dynamic_array(&g_array, 100, sizeof(Message));

    // --------------------------------------------- Client part -------------------------------------------------------

    uint64_t starting_value = 2500;
    uint64_t ending_value = 5000;
    uint64_t diff = ending_value - starting_value;

    // --------------------------------------------- Sender part -------------------------------------------------------
    for (uint64_t i = starting_value; i < ending_value; i++) {
        char *custom_message = (char *) malloc(20 * sizeof(char));
        sprintf(custom_message, "[Value: %llu]", i);
        set_message_id(i);  // Only for testing purposes (to avoid generating random IDs)
        Message *msg = create_element(custom_message);
        if (msg == NULL) continue;

        // fake timeout or will fail next tests
        msg->timestamp = msg->timestamp - 6000; // make it old
        add_to_dynamic_array(&g_array, msg);
        free(custom_message);
    }

    // --------------------------------------------- Responder part ----------------------------------------------------
    // Create a dynamic buffer
    DynamicArray g_array2;
    init_dynamic_array(&g_array2, 100, sizeof(Message));
    uint64_t count = 0;

    // In the range of starting_value and ending_value I'll add the 80% of the messages
    for (uint64_t i = starting_value, j = 0; i < ending_value; i++, j++) {
        if (j % 5 == 0) {
            char *custom_message = (char *) malloc(20 * sizeof(char));
            sprintf(custom_message, "[Value2: %llu]", i);
            set_message_id(i);  // Only for testing purposes (to avoid generating random IDs)
            Message *msg = create_element(custom_message);
            if (msg == NULL) continue;
            add_to_dynamic_array(&g_array2, msg);
            free(custom_message);
            count++;
        }
    }

    // Messages received (from buffer)
    char *buffer = marshal_uint64_array(&g_array2);
    // printf("\n The BUFFER: %s\n", buffer);

    // Retrieve all messages ids sent from the client to the server
    DynamicArray *new_array = unmarshal_uint64_array(buffer);
    if (new_array == NULL) {
        return;
    }

    int missed_count = diff_from_arrays(&g_array, new_array, NULL);

    char *buffer1 = marshal_uint64_array(&g_array);
    // printf("\n The BUFFER1: %s\n", buffer1);
    free(buffer1);


    // Check if missed_count == (ending_value - starting_value) - count
    TEST_ASSERT_EQUAL_INT(missed_count, diff - count);
}


// The main function for running the tests
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_process_missed_message_ids);
    RUN_TEST(test_buffer_and_get_missed_ids);
    RUN_TEST(test_client_server_missing_ids);
    RUN_TEST(test_big_differences);
    UNITY_END();

    check_for_leaks();  // Check for memory leaks
    return 0;
}
