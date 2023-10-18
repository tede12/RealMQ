#include "unity.h"
#include "utils/utils.h"
#include "string_manip.h"

char **generate_uuids(size_t count) {
    char **uuids = (char **) malloc(count * sizeof(char *));
    for (size_t i = 0; i < count; ++i) {
        uuids[i] = (char *) malloc(37);
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
    // Clean up message_ids
    for (size_t i = 0; i < num_message_ids; ++i) {
        free(message_ids[i]);
    }
    free(message_ids);
    message_ids = NULL;
    num_message_ids = 0;

    // Destroy mutex
    pthread_mutex_destroy(&message_ids_mutex);
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
}

// The main function for running the tests
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_process_missed_message_ids);
    // todo add more test cases here

    return UNITY_END();
}
