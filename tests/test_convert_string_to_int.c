#include "unity.h"
#include "core/config.h"

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void test_function_convert_valid_string_to_int(void) {
    TEST_ASSERT_EQUAL_INT(123, convert_string_to_int("123"));
}

void test_function_convert_invalid_string_to_int(void) {
    TEST_ASSERT_EQUAL_INT(-1, convert_string_to_int("abc"));
}

void test_function_convert_empty_string_to_int(void) {
    TEST_ASSERT_EQUAL_INT(0, convert_string_to_int(""));
}

void test_function_convert_string_with_overflow_to_int(void) {
    TEST_ASSERT_EQUAL_INT(-1, convert_string_to_int("9999999999999999999999999")); // this value is too big for an int
}

// the main function for running the tests
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_function_convert_valid_string_to_int);
    RUN_TEST(test_function_convert_invalid_string_to_int);
    RUN_TEST(test_function_convert_empty_string_to_int);
    RUN_TEST(test_function_convert_string_with_overflow_to_int);

    return UNITY_END();
}
