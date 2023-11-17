#include "unity.h"
#include <stdlib.h>
#include "qos/dynamic_array.h"
#include <inttypes.h>

// Create a new dynamic array
DynamicArray g_array;


void setUp(void) {}

void tearDown(void) {
    release_dynamic_array(&g_array);
}

void test_uint64_t_array_marshalling(void) {
    // Initialize the dynamic array
    init_dynamic_array(&g_array, 5, sizeof(uint64_t));

    // Populate the array with some test data
    for (uint64_t i = 0; i < g_array.capacity; ++i) {
        uint64_t *msg_id = malloc(sizeof(uint64_t));
        *msg_id = i;
        add_to_dynamic_array(&g_array, msg_id);
        free(msg_id); // Free after adding since add_to_dynamic_array makes a deep copy
    }

    // Marshal the array
    char *marshalled_buffer = marshal_uint64_array(&g_array);

    // Check if marshalling was successful
    TEST_ASSERT_NOT_NULL_MESSAGE(marshalled_buffer, "Marshalling returned NULL");

    // Unmarshal the data back into a dynamic array
    DynamicArray *unmarshalled_array = unmarshal_uint64_array(marshalled_buffer);

    // Check if unmarshalling was successful
    TEST_ASSERT_NOT_NULL_MESSAGE(unmarshalled_array, "Unmarshalling returned NULL");

    // New array should be pointing to a different memory location than the original array
    TEST_ASSERT_NOT_EQUAL(&g_array, unmarshalled_array);

    // Assert that the unmarshalled data matches the original data
    for (size_t i = 0; i < unmarshalled_array->size; ++i) {

        uint64_t *msg_id = (uint64_t *) unmarshalled_array->data[i];
        uint64_t *msg_id2 = (uint64_t *) g_array.data[i];

//        // ONLY FOR DEBUGGING
//        printf("unmarshalled_array->data[%zu] = %" PRIu64 "\n", i, *msg_id);
//        printf("g_array.data[%zu] = %" PRIu64 "\n", i, *msg_id2);

        TEST_ASSERT_EQUAL_UINT64(*msg_id, *msg_id2);
    }

    // Free the resources
    free(marshalled_buffer);
    release_dynamic_array(unmarshalled_array);
}


void test_marshalling_message(uint64_t msg_id, const char *expected_content) {
    Message *msg = create_element(expected_content);
    msg->id = msg_id;

    // Marshal the message
    const char *buffer = marshal_message(msg);
    if (buffer == NULL) {
        TEST_FAIL_MESSAGE("Marshalling failed");
    }

    // Unmarshal the message
    Message *new_msg = unmarshal_message(buffer);
    if (new_msg == NULL) {
        TEST_FAIL_MESSAGE("Unmarshalling failed");
    }

    // New message should be pointing to a different memory location than the original message
    TEST_ASSERT_NOT_EQUAL(msg, new_msg);

    TEST_ASSERT_EQUAL_STRING(expected_content, new_msg->content);
    TEST_ASSERT_EQUAL_UINT64(msg_id, new_msg->id);

    // Free the resources
    release_element(msg, sizeof(Message));
    release_element(new_msg, sizeof(Message));
    free((void *) buffer);

}

void test_marshalling_message_1_hello(void) {
    test_marshalling_message(1, "Hello");
}

void test_marshalling_message_2000_world(void) {
    test_marshalling_message(2000, "World");
}

void test_marshalling_message_99999_hello_world(void) {
    test_marshalling_message(99999, "Hello World");
}


int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_marshalling_message_1_hello);             // Message marshalling
    RUN_TEST(test_marshalling_message_2000_world);          // Message marshalling
    RUN_TEST(test_marshalling_message_99999_hello_world);   // Message marshalling
    RUN_TEST(test_uint64_t_array_marshalling);              // uint64_t array marshalling

    // More RUN_TEST() calls...
    return UNITY_END();
}
