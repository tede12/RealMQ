#include "unity.h"
#include <stdlib.h>
#include "qos/dynamic_array.h"
#include "qos/interpolation_search.h"

// Create a new dynamic array
DynamicArray g_array;

void setUp(void) {
    // Populate the array with some values
    init_dynamic_array(&g_array, 100000);

    for (int i = 0; i < 100000; i++) {
        add_to_dynamic_array(&g_array, i);
    }
//    // Print the array
//    print_dynamic_array(&array);
}

void tearDown(void) {
    // This is run after EACH test
    // Clean up allocated memory
    release_dynamic_array(&g_array);
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
    test_if_msg_id_exist(0, 0);
}

void test_msg_id_1000_exists(void) {
    test_if_msg_id_exist(1000, 1000);
}

void test_msg_id_99999_exists(void) {
    test_if_msg_id_exist(99999, 99999);
}

void test_msg_id_100000_does_not_exist(void) {
    test_if_msg_id_exist(100000, -1);
}

void test_msg_id_100001_does_not_exist(void) {
    test_if_msg_id_exist(100001, -1);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_msg_id_0_exists);
    RUN_TEST(test_msg_id_1000_exists);
    RUN_TEST(test_msg_id_99999_exists);
    RUN_TEST(test_msg_id_100000_does_not_exist);
    RUN_TEST(test_msg_id_100001_does_not_exist);
    // More RUN_TEST() calls...
    return UNITY_END();
}
