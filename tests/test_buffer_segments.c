#include "unity.h"
#include "../common/qos/buffer_segments.h"
#include "../common/utils/memory_leak_detector.h"

void setUp(void) {
    MAX_SEGMENT_SIZE = 11;
}

void tearDown(void) {
    MAX_SEGMENT_SIZE = 1024;
}

void test_marshal_and_split(void) {
    // Create a DynamicArray and populate it with test data
    DynamicArray testArray;
    init_dynamic_array(&testArray, 3, sizeof(uint64_t));
    uint64_t values[] = {12345, 67890, 23456}; // Example values
    for (size_t i = 0; i < 3; i++) {
        add_to_dynamic_array(&testArray, &values[i]);
    }

    char *res = marshal_uint64_array(&testArray); // Marshal the array (this will set the array's buffer

    // Call the function to test
    BufferSegmentArray result = marshal_and_split(&testArray);

    // Check if the result is as expected
    TEST_ASSERT_NOT_NULL(result.segments);
    TEST_ASSERT_TRUE(result.count > 1); // There should be more than one segment

    // Check each segment to ensure it does not exceed the maximum size
    for (size_t i = 0; i < result.count; i++) {
        TEST_ASSERT_LESS_OR_EQUAL_UINT(MAX_SEGMENT_SIZE, result.segments[i].size);
    }

    // Clean up
    free_segment_array(&result);
    release_dynamic_array(&testArray);
}

void test_marshal_and_split_empty_array(void) {
    DynamicArray testArray;
    init_dynamic_array(&testArray, 0, sizeof(uint64_t)); // Empty array

    BufferSegmentArray result = marshal_and_split(&testArray);

    TEST_ASSERT_NULL(result.segments);
    TEST_ASSERT_EQUAL_UINT(0, result.count); // Expecting no segments for an empty array

    free_segment_array(&result);
    release_dynamic_array(&testArray);
}

void test_marshal_and_split_single_large_number(void) {
    DynamicArray testArray;
    init_dynamic_array(&testArray, 1, sizeof(uint64_t));
    uint64_t value = 123456789012345; // Large number
    add_to_dynamic_array(&testArray, &value);

    BufferSegmentArray result = marshal_and_split(&testArray);

    TEST_ASSERT_NULL(result.segments);
    TEST_ASSERT_TRUE(result.count == 0); // Should be a 0 segment

    free_segment_array(&result);
    release_dynamic_array(&testArray);
}

void test_marshal_and_split_many_small_numbers(void) {
    DynamicArray testArray;
    init_dynamic_array(&testArray, 10, sizeof(uint64_t));
    uint64_t values[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}; // Small numbers
    for (size_t i = 0; i < 10; i++) {
        add_to_dynamic_array(&testArray, &values[i]);
    }

    BufferSegmentArray result = marshal_and_split(&testArray);

    TEST_ASSERT_NOT_NULL(result.segments);
    TEST_ASSERT_TRUE(result.count > 1); // Should be split into multiple segments

    free_segment_array(&result);
    release_dynamic_array(&testArray);
}


int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_marshal_and_split);
    RUN_TEST(test_marshal_and_split_empty_array);
    RUN_TEST(test_marshal_and_split_single_large_number);
    RUN_TEST(test_marshal_and_split_many_small_numbers);
    UNITY_END();

    check_for_leaks();  // Check for memory leaks
    return 0;
}
