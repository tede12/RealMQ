#include "unity.h"
#include "qos/accrual_detector/state.h"
#include "utils/time_utils.h"
#include "utils/memory_leak_detector.h"

void setUp(void) {
    // This function is run before each test
}

void tearDown(void) {
    // This function is run after each test
}

void test_state_init(void) {
    long long timestamp = 1234567890; // Use a fixed timestamp for testing
    state_t *state = state_init(NULL, timestamp);

    // Verify that state is not NULL
    TEST_ASSERT_NOT_NULL(state);

    // Verify that state's history is not NULL (since we passed NULL, it should initialize a new history)
    TEST_ASSERT_NOT_NULL(state->history);

    // Verify that the timestamp is set correctly
    TEST_ASSERT_EQUAL_INT64(timestamp, state->timestamp);

    // Clean up
    delete_state(state, true);
}

void test_state_init_with_zero_timestamp(void) {
    // Test with zero timestamp (state_init should then get the current time)
    state_t *state = state_init(NULL, 0);

    // Verify that state is not NULL
    TEST_ASSERT_NOT_NULL(state);

    // Verify that state's history is not NULL
    TEST_ASSERT_NOT_NULL(state->history);

    TEST_ASSERT_EQUAL(0, state->timestamp);

    // Clean up
    delete_state(state, true);
}

void test_compare_states_equal(void) {
    long long timestamp = get_current_timestamp();
    heartbeat_history_t *history1 = new_heartbeat_history(1, NULL, 0, 0.f, 0.f);
    state_t *state1 = state_init(history1, timestamp);

    // Create a second identical state
    heartbeat_history_t *history2 = new_heartbeat_history(1, NULL, 0, 0.f, 0.f);
    state_t *state2 = state_init(history2, timestamp);

    // Verify that the states are considered equal
    TEST_ASSERT_TRUE(compare_states(state1, state2));

    // Clean up
    delete_state(state1, true);
    delete_state(state2, true);
    delete_heartbeat_history(history1);
    delete_heartbeat_history(history2);
}

void test_compare_states_not_equal(void) {
    // Diff by TIMESTAMP
    long long timestamp1 = get_current_timestamp();
    heartbeat_history_t *history1 = new_heartbeat_history(1, NULL, 0, 0.f, 0.f);
    state_t *state1 = state_init(history1, timestamp1);

    // Create a second state with a different timestamp
    long long timestamp2 = timestamp1 + 1000; // 1 second later
    heartbeat_history_t *history2 = new_heartbeat_history(1, NULL, 0, 0.f, 0.f);
    state_t *state2 = state_init(history2, timestamp2);

    // Verify that the states are not considered equal due to different timestamps
    TEST_ASSERT_FALSE(compare_states(state1, state2));

    // Clean up
    delete_state(state1, true);
    delete_state(state2, true);
    delete_heartbeat_history(history1);
    delete_heartbeat_history(history2);

    // Diff by HISTORY
    timestamp1 = get_current_timestamp();
    history1 = new_heartbeat_history(1, NULL, 0, 0.f, 0.f);
    state1 = state_init(history1, timestamp1);

    // Create a second state with a different history
    timestamp2 = timestamp1;
    history2 = new_heartbeat_history(2, NULL, 0, 0.f, 0.f);
    state2 = state_init(history2, timestamp2);

    // Verify that the states are not considered equal due to different histories
    TEST_ASSERT_FALSE(compare_states(state1, state2));

    // Clean up
    delete_state(state1, true);
    delete_state(state2, true);
    delete_heartbeat_history(history1);
    delete_heartbeat_history(history2);

    // Diff by NULL
    timestamp1 = get_current_timestamp();
    history1 = new_heartbeat_history(1, NULL, 0, 0.f, 0.f);
    state1 = state_init(history1, timestamp1);

    // Create a second state with a NULL history
    timestamp2 = timestamp1;
    state2 = state_init(NULL, timestamp2);
    // Due to the NULL state, the history should be initialized to a new history. So I force it to NULL
    delete_heartbeat_history(state2->history);
    state2->history = NULL;

    // Verify that the states are not considered equal due to different histories
    TEST_ASSERT_FALSE(compare_states(state1, state2));

    // Clean up
    delete_state(state1, true);
    delete_state(state2, true);
    delete_heartbeat_history(history1);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_state_init);
    RUN_TEST(test_state_init_with_zero_timestamp);
    RUN_TEST(test_compare_states_equal);
    RUN_TEST(test_compare_states_not_equal);
    check_for_leaks();
    UNITY_END();
}
