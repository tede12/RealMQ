#include "unity.h"
#include <stdlib.h>
#include "qos/dynamic_array.h"


// Create a new dynamic array
DynamicArray g_array;


void setUp(void) {}

void tearDown(void) {
    release_dynamic_array(&g_array);
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

    RUN_TEST(test_marshalling_message_1_hello);
    RUN_TEST(test_marshalling_message_2000_world);
    RUN_TEST(test_marshalling_message_99999_hello_world);

    // More RUN_TEST() calls...
    return UNITY_END();
}
