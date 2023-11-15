#include "unity.h"
#include <stdlib.h>
#include "qos/dynamic_array.h"
#include "qos/interpolation_search.h"

#define MAX_CAPACITY 100000

// Create a new dynamic array
DynamicArray g_array;


void setUp(void) {}

void tearDown(void) {}

void test_marshalling_message(uint64_t msg_id, const char *expected_content) {
    Message *msg = create_message(expected_content);
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
    release_message(msg);
    release_message(new_msg);
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

void populate_array(void) {
    // Populate the array with some values
    init_dynamic_array(&g_array, MAX_CAPACITY);    // 100000

    for (uint64_t i = 0; i < MAX_CAPACITY; i++) {
        Message *msg = create_message("Hello world");
        msg->id = i;
        add_message_to_dynamic_array(&g_array, *msg);

        release_message(msg);
    }
    // print_dynamic_array(&g_array);
}

void test_if_msg_id_exist(long long msg_id, long long expected_index) {
    // If expected_index is -1, then the msg_id should not exist in the array
    long long index = interpolate_search(&g_array, msg_id);

    if (expected_index == -1) {
        TEST_ASSERT_EQUAL_INT(-1, index);
    } else {
        TEST_ASSERT_EQUAL_INT(expected_index, index);
    }
}

void test_msg_id_0_exists(void) {
    populate_array();
    test_if_msg_id_exist(0, 0);
    release_dynamic_array(&g_array);
}

void test_msg_id_1000_exists(void) {
    populate_array();
    test_if_msg_id_exist(1000, 1000);
    release_dynamic_array(&g_array);
}

void test_msg_id_99999_exists(void) {
    populate_array();
    test_if_msg_id_exist(99999, 99999);
    release_dynamic_array(&g_array);
}

void test_msg_id_100000_does_not_exist(void) {
    populate_array();
    test_if_msg_id_exist(100000, -1);
    release_dynamic_array(&g_array);
}

void test_msg_id_100001_does_not_exist(void) {
    populate_array();
    test_if_msg_id_exist(100001, -1);
    release_dynamic_array(&g_array);
}


void test_removing_ids(void) {
    populate_array();
    // Remove randomly 100 IDs from the array and check if they exist

    // Create a list of 100 random IDs
    uint64_t random_ids[100];
    for (int i = 0; i < 100; i++) {
        random_ids[i] = rand() % MAX_CAPACITY;
    }

    // Remove the IDs from the array
    for (int i = 0; i < 100; i++) {
        remove_message_by_id(&g_array, random_ids[i], true);
    }

    // Check the length of the array
    TEST_ASSERT_EQUAL_INT(MAX_CAPACITY - 100, g_array.size);

    // Check if the IDs exist in the array
    bool found = false;
    for (int i = 0; i < 100; i++) {
        long long index = interpolate_search(&g_array, random_ids[i]);
        if (index != -1) {
            found = true;
            break;
        }
    }

    TEST_ASSERT_FALSE(found);

    release_dynamic_array(&g_array);
}


int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_msg_id_0_exists);
    RUN_TEST(test_msg_id_1000_exists);
    RUN_TEST(test_msg_id_99999_exists);
    RUN_TEST(test_msg_id_100000_does_not_exist);
    RUN_TEST(test_msg_id_100001_does_not_exist);

    RUN_TEST(test_removing_ids);

    RUN_TEST(test_marshalling_message_1_hello);
    RUN_TEST(test_marshalling_message_2000_world);
    RUN_TEST(test_marshalling_message_99999_hello_world);

    // More RUN_TEST() calls...
    return UNITY_END();
}
