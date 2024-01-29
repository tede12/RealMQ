#include "unity.h"
#include <stdlib.h>
#include "qos/dynamic_array.h"
#include "qos/interpolation_search.h"
#include <inttypes.h>

#define MAX_CAPACITY 100000     // KEEP THIS VALUE FOR TESTING (or some test would fail)

// Create a new dynamic array
DynamicArray g_array;


void setUp(void) {
    // Reset the atomic message ID for each test
    atomic_msg_id = 0;
}

void tearDown(void) {
    release_dynamic_array(&g_array);
}

void populate_array(size_t element_size) {
    // Populate the array with some values
    if (element_size == sizeof(Message)) {
        init_dynamic_array(&g_array, MAX_CAPACITY, sizeof(Message));
    } else if (element_size == sizeof(uint64_t)) {
        init_dynamic_array(&g_array, MAX_CAPACITY, sizeof(uint64_t));
    } else {
        TEST_FAIL_MESSAGE("Unsupported element size");
    }

    for (uint64_t i = 0; i < MAX_CAPACITY; i++) {

        if (element_size == sizeof(Message)) {
            Message *msg = create_element("Hello world");
            add_to_dynamic_array(&g_array, msg);
            release_element(msg, sizeof(Message));
        } else {
            uint64_t *msg_id = create_element(NULL);
            add_to_dynamic_array(&g_array, msg_id);
            release_element(msg_id, sizeof(uint64_t));
        }
    }
    // print_dynamic_array(&g_array);
}

void test_if_msg_id_exist(long long msg_id, long long expected_index) {
    // If expected_index is -1, then the msg_id should not exist in the array
    long long index = interpolate_search(&g_array, msg_id);

//    // ONLY FOR DEBUGGING
//    char *element_type;
//    if (g_array.element_size == sizeof(Message)) {
//        element_type = "Message";
//    } else if (g_array.element_size == sizeof(uint64_t)) {
//        element_type = "uint64_t";
//    } else {
//        element_type = "Unknown";
//    }
//
//    printf("Attempting test with Type: %s, ID: %lld, Expected: %lld, Actual: %lld\n",
//           element_type, msg_id, expected_index, index);

    if (expected_index == -1) {
        TEST_ASSERT_EQUAL_INT(-1, index);
    } else {
        TEST_ASSERT_EQUAL_INT(expected_index, index);
    }
}

void test_msg_id_1_exists_(size_t element_size) {
    populate_array(element_size);
    test_if_msg_id_exist(1, 0);
}

void test_msg_id_1000_exists_(size_t element_size) {
    populate_array(element_size);
    test_if_msg_id_exist(1000, 999);

}

void test_msg_id_99999_exists_(size_t element_size) {
    populate_array(element_size);
    test_if_msg_id_exist(99999, 99998);

}

void test_msg_id_100001_does_not_exist_(size_t element_size) {
    populate_array(element_size);
    test_if_msg_id_exist(100001, -1);

}

void test_msg_id_200000_does_not_exist_(size_t element_size) {
    populate_array(element_size);
    test_if_msg_id_exist(200000, -1);

}


void test_removing_ids_(size_t element_size) {
    populate_array(element_size);
    // Remove randomly 100 IDs from the array and check if they exist

    // Create a list of 100 random IDs
    uint64_t random_ids[100];
    for (int i = 0; i < 100; i++) {
        random_ids[i] = rand() % MAX_CAPACITY;
    }

    // Remove the IDs from the array
    for (int i = 0; i < 100; i++) {
        remove_element_by_id(&g_array, random_ids[i], true, true);
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


}


void test_get_element_by_index(void) {
    populate_array(sizeof(uint64_t));

    // Index starts from 0, msg_id starts from 1
    uint64_t *msg_id = get_element_by_index(&g_array, 0);

    TEST_ASSERT_EQUAL_INT(1, *msg_id);

    msg_id = get_element_by_index(&g_array, 999);
    TEST_ASSERT_EQUAL_INT(1000, *msg_id);

    msg_id = get_element_by_index(&g_array, 99998);
    TEST_ASSERT_EQUAL_INT(99999, *msg_id);

    msg_id = get_element_by_index(&g_array, 99999);
    TEST_ASSERT_EQUAL_INT(100000, *msg_id);

    // This should return NULL
    msg_id = get_element_by_index(&g_array, 100000);
    TEST_ASSERT_NULL(msg_id);

    // Test negative index
    msg_id = get_element_by_index(&g_array, -1);
    TEST_ASSERT_EQUAL_INT(100000, *msg_id);

    msg_id = get_element_by_index(&g_array, -2);
    TEST_ASSERT_EQUAL_INT(99999, *msg_id);
}

void test_get_element_by_index_using_random(void) {
    // Populate using random ids
    uint64_t random_ids[100];
    for (int i = 0; i < 100; i++) {
        random_ids[i] = rand() % MAX_CAPACITY;
    }

    // Populate the array with some values
    init_dynamic_array(&g_array, MAX_CAPACITY, sizeof(uint64_t));
    for (uint64_t i = 0; i < 100; i++) {
        uint64_t msg_id = random_ids[i];
        add_to_dynamic_array(&g_array, &msg_id);
    }

    // Check if the values are correct
    for (int i = 0; i < 100; i++) {
        uint64_t *msg_id = get_element_by_index(&g_array, i);
        TEST_ASSERT_EQUAL_INT(random_ids[i], *msg_id);
    }
}


void test_msg_id_1_exists_message(void) {
    test_msg_id_1_exists_(sizeof(Message));
}

void test_msg_id_1_exists_value(void) {
    test_msg_id_1_exists_(sizeof(uint64_t));
}

void test_msg_id_1000_exists_message(void) {
    test_msg_id_1000_exists_(sizeof(Message));
}

void test_msg_id_1000_exists_value(void) {
    test_msg_id_1000_exists_(sizeof(uint64_t));
}

void test_msg_id_99999_exists_message(void) {
    test_msg_id_99999_exists_(sizeof(Message));
}

void test_msg_id_99999_exists_value(void) {
    test_msg_id_99999_exists_(sizeof(uint64_t));
}

void test_msg_id_100001_does_not_exist_message(void) {
    test_msg_id_100001_does_not_exist_(sizeof(Message));
}

void test_msg_id_100001_does_not_exist_value(void) {
    test_msg_id_100001_does_not_exist_(sizeof(uint64_t));
}

void test_msg_id_200000_does_not_exist_message(void) {
    test_msg_id_200000_does_not_exist_(sizeof(Message));
}

void test_msg_id_200000_does_not_exist_value(void) {
    test_msg_id_200000_does_not_exist_(sizeof(uint64_t));
}

void test_removing_ids_message(void) {
    test_removing_ids_(sizeof(Message));
}

void test_removing_ids_value(void) {
    test_removing_ids_(sizeof(uint64_t));
}


int main(void) {
    UNITY_BEGIN();
    // Tests for interpolation search
    RUN_TEST(test_msg_id_1_exists_message);
    RUN_TEST(test_msg_id_1_exists_value);
    RUN_TEST(test_msg_id_1000_exists_message);
    RUN_TEST(test_msg_id_1000_exists_value);
    RUN_TEST(test_msg_id_99999_exists_message);
    RUN_TEST(test_msg_id_99999_exists_value);
    RUN_TEST(test_msg_id_100001_does_not_exist_message);
    RUN_TEST(test_msg_id_100001_does_not_exist_value);
    RUN_TEST(test_msg_id_200000_does_not_exist_message);
    RUN_TEST(test_msg_id_200000_does_not_exist_value);

    // Tests for removing IDs
    RUN_TEST(test_removing_ids_message);
    RUN_TEST(test_removing_ids_value);

    // Tests for get_element_by_index
    RUN_TEST(test_get_element_by_index);
    RUN_TEST(test_get_element_by_index_using_random);
    // More RUN_TEST() calls...
    return UNITY_END();
}
