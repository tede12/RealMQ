#include "unity.h"
#include "string_manip.h"

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void test_string_split_function(void) {
    const char *str = "5FE2B0E8-EE48-4DF9-92CD-51214ECEBCD7,FF8EC035-49BC-4192-8D24-97DCF0240D9F,BB7EACCB-2A85-4F6A-9983-EF1477D1B23B,1A76D026-E988-431F-93A9-38728213E50E";
    size_t num_tokens;
    char **tokens = split_string(str, ',', &num_tokens);

    TEST_ASSERT_EQUAL_INT(4, num_tokens); // Expecting 4 tokens

    if (num_tokens == 4) { // Check to prevent potential segmentation fault if the number of tokens isn't as expected
        TEST_ASSERT_EQUAL_STRING("5FE2B0E8-EE48-4DF9-92CD-51214ECEBCD7", tokens[0]);
        TEST_ASSERT_EQUAL_STRING("FF8EC035-49BC-4192-8D24-97DCF0240D9F", tokens[1]);
        TEST_ASSERT_EQUAL_STRING("BB7EACCB-2A85-4F6A-9983-EF1477D1B23B", tokens[2]);
        TEST_ASSERT_EQUAL_STRING("1A76D026-E988-431F-93A9-38728213E50E", tokens[3]);
    }

    for (size_t i = 0; i < num_tokens; i++) {
        free(tokens[i]); // Free each string
    }
    free(tokens); // And the array itself
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_string_split_function);
    return UNITY_END();
}
