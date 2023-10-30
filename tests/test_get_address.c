#include "unity.h"
#include "core/config.h" // assuming this header file contains the declaration of get_address function


void setUp(void) {
    // set stuff up here
    if (read_config("../config.yaml") != 0) {
        TEST_FAIL_MESSAGE("Failed to read config.yaml");
        TEST_ASSERT_EQUAL_INT(0, 1); // force the test to fail
    }
}

void tearDown(void) {
    // clean stuff up here
}

void test_function_get_valid_address(void) {
    config.responder_address = "127.0.0.1:8000";
    config.protocol = "tcp";

    char *address = get_address(RESPONDER_ADDRESS);
    TEST_ASSERT_NOT_NULL(address);
    TEST_ASSERT_EQUAL_STRING("tcp://127.0.0.1:8000", address);
}

void test_function_get_invalid_address_type(void) {
    char *address = get_address(9999); // assuming 9999 is an invalid AddressType
    TEST_ASSERT_NULL(address);
}

// Add more tests as needed for different cases

// the main function for running the tests
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_function_get_valid_address);
    RUN_TEST(test_function_get_invalid_address_type);
    // other test cases here
    release_config();
    return UNITY_END();
}
