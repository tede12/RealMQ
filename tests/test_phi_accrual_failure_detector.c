#include "unity.h"
#include "qos/accrual_detector/phi_accrual_failure_detector.h"
#include "utils/time_utils.h"
#include "core/logger.h"
#include "utils/memory_leak_detector.h"

Logger test_logger;

void setUp(void) {
    // Set up before each test
}

void tearDown(void) {
    // Clean up after each test
    fake_time = 0; // Reset the fake time
}

void test_new_phi_accrual_detector(void) {
    float threshold = 1.5f;
    size_t max_sample_size = 5;
    float min_std_deviation_ms = 0.1f;
    float acceptable_heartbeat_pause_ms = 1.0f;
    float first_heartbeat_estimate_ms = 0.5f;

    phi_accrual_detector *detector = new_phi_accrual_detector(
            threshold, max_sample_size, min_std_deviation_ms, acceptable_heartbeat_pause_ms,
            first_heartbeat_estimate_ms, NULL);

    TEST_ASSERT_NOT_NULL(detector);
    TEST_ASSERT_EQUAL_FLOAT(threshold, detector->threshold);
    TEST_ASSERT_EQUAL_UINT(max_sample_size, detector->max_sample_size);
    TEST_ASSERT_EQUAL_FLOAT(min_std_deviation_ms, detector->min_std_deviation_ms);
    TEST_ASSERT_EQUAL_FLOAT(acceptable_heartbeat_pause_ms, detector->acceptable_heartbeat_pause_ms);
    TEST_ASSERT_EQUAL_FLOAT(first_heartbeat_estimate_ms, detector->first_heartbeat_estimate_ms);
    TEST_ASSERT_NOT_NULL(detector->state);
    TEST_ASSERT_NOT_NULL(detector->state->history);

    delete_phi_accrual_detector(detector);
}

void test_ensure_valid_std_deviation(void) {
    phi_accrual_detector *detector = new_phi_accrual_detector(1.5f, 5, 0.1f, 1.0f, 0.5f, NULL);

    float std_deviation = 0.05f;
    float result = ensure_valid_std_deviation(detector, std_deviation);
    TEST_ASSERT_EQUAL_FLOAT(detector->min_std_deviation_ms, result);

    std_deviation = 0.2f;
    result = ensure_valid_std_deviation(detector, std_deviation);
    TEST_ASSERT_EQUAL_FLOAT(std_deviation, result);

    delete_phi_accrual_detector(detector);
}

void test_is_available(void) {
    float threshold = 1.5f;
    size_t max_sample_size = 5;
    float min_std_deviation_ms = 0.1f;
    float acceptable_heartbeat_pause_ms = 1.0f;
    float first_heartbeat_estimate_ms = 0.5f;

    phi_accrual_detector *detector = new_phi_accrual_detector(
            threshold, max_sample_size, min_std_deviation_ms, acceptable_heartbeat_pause_ms,
            first_heartbeat_estimate_ms, NULL);

    // Simulate a condition for phi value below the threshold
    // For simplicity, let's assume the detector's state can be directly manipulated
    // In a real-world scenario, you might need to simulate heartbeats over time to achieve this
    detector->state->timestamp = get_current_timestamp() - 100; // Simulate a recent heartbeat
    // Assuming `add_interval` and `get_current_timestamp` are mockable or can be controlled for the test
    add_interval(detector->state->history, 0.1f); // Add a small interval

    // Check when phi value is likely above threshold
    TEST_ASSERT_TRUE(!is_available(detector, 0));

    // Now simulate a condition for phi value below the threshold
    detector->state->timestamp = get_current_time_millis() - 10000; // Simulate a very old heartbeat
    add_interval(detector->state->history, 10.0f); // Add a large interval

    // Check when phi value is likely above threshold
    TEST_ASSERT_FALSE(is_available(detector, 0));

    delete_phi_accrual_detector(detector);
}


// fake_time could be set and used to simulate time, so if you call get_current_timestamp() it will return fake_time
void test_get_phi(void) {
#undef DEBUG_TIMES
    // Test parameters
    fake_time = 900;
    float threshold = 1.5f;
    size_t max_sample_size = 5;
    float min_std_deviation_ms = 0.1f;
    float acceptable_heartbeat_pause_ms = 1.0f;
    float first_heartbeat_estimate_ms = 0.5f;

    // Create a phi accrual detector
    phi_accrual_detector *detector = new_phi_accrual_detector(
            threshold, max_sample_size, min_std_deviation_ms,
            acceptable_heartbeat_pause_ms, first_heartbeat_estimate_ms, NULL);

    // Test case 1: No state initialized, so phi should be 0
    fake_time = 1000;
    float phi_no_heartbeat = get_phi(detector, fake_time);
    TEST_ASSERT_EQUAL_FLOAT(0.f, phi_no_heartbeat);

    // Test case 2: Recent heartbeat
    fake_time = 1100;
    add_interval(detector->state->history, 100); // Adding a heartbeat 100ms ago
    detector->state->timestamp = 1000;
    float phi_recent_heartbeat = get_phi(detector, fake_time);
    // Expected value calculation based on your algorithm
    float expected_phi_recent = 1.2f; // Calculate expected value
    TEST_ASSERT_FLOAT_WITHIN(0.1f, expected_phi_recent, phi_recent_heartbeat);

    // Test case 3: Old heartbeat
    fake_time = 2000;
    add_interval(detector->state->history, 900); // Adding a heartbeat 900ms ago
    detector->state->timestamp = 1100;
    float phi_old_heartbeat = get_phi(detector, fake_time);
    // Expected value calculation based on your algorithm
    float expected_phi_old = 1.4f; // Calculate expected value
    TEST_ASSERT_FLOAT_WITHIN(0.1f, expected_phi_old, phi_old_heartbeat);

    // Clean up
    delete_phi_accrual_detector(detector);
#define DEBUG_TIMES
}


void test_compare_and_set(void) {
    // Create an initial state
    heartbeat_history_t *initial_history = new_heartbeat_history(5, NULL, 0, 0.f, 0.f);
    logger(LOG_LEVEL_INFO2, "initial_history: %p, initial_history->intervals: %p", initial_history,
           initial_history->intervals);
    state_t *initial_state = state_init(initial_history, 1000);
    logger(LOG_LEVEL_INFO2, "initial_state: %p", initial_state);

    // Create an expected state which should match the current state
    heartbeat_history_t *expected_history = new_heartbeat_history(5, NULL, 0, 0.f, 0.f);
    logger(LOG_LEVEL_INFO2, "expected_history: %p, expected_history->intervals: %p", expected_history,
           expected_history->intervals);
    state_t *expected_state = state_init(expected_history, 1000);
    logger(LOG_LEVEL_INFO2, "expected_state: %p", expected_state);

    // Create a new state for updating
    heartbeat_history_t *new_history = new_heartbeat_history(5, NULL, 0, 0.f, 0.f);
    logger(LOG_LEVEL_INFO2, "new_history: %p, new_history->intervals: %p", new_history, new_history->intervals);
    state_t *new_state = state_init(new_history, 2000);
    logger(LOG_LEVEL_INFO2, "new_state: %p", new_state);
    TEST_ASSERT_EQUAL_INT64(initial_state->timestamp, expected_state->timestamp);

    // Test successful compare and set
    bool success = compare_and_set(
            initial_state,
            expected_state,
            new_state
    );  // this should update initial_state to new_state because it matches expected_state

    // this should free expected_state and new_state and keep only initial_state (which should be updated to new_state)
    logger(LOG_LEVEL_INFO2, "success: %d", success);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL_INT64(initial_state->timestamp, new_state->timestamp);
    TEST_ASSERT_NOT_EQUAL_INT64(initial_state->timestamp, expected_state->timestamp);

    // Create another new state for updating
    heartbeat_history_t *another_new_history = new_heartbeat_history(5, NULL, 0, 0.f, 0.f);
    state_t *another_new_state = state_init(another_new_history, 3000);

    // Modify the initial state to simulate change
    initial_state->timestamp = 1500;
    TEST_ASSERT_NOT_EQUAL_INT64(initial_state->timestamp, expected_state->timestamp);

    // Test failed compare and set
    success = compare_and_set(initial_state, expected_state, another_new_state);
    TEST_ASSERT_FALSE(success);
    TEST_ASSERT_NOT_EQUAL_INT64(initial_state->timestamp, another_new_state->timestamp);

    // Clean up
    logger(LOG_LEVEL_DEBUG,
           "initial_state: %p, initial_state->history: %p, initial_state->history->intervals: %p",
           initial_state, initial_state->history, initial_state->history->intervals);
    delete_state(initial_state, true);

    logger(LOG_LEVEL_DEBUG,
           "new_state: %p, new_state->history: %p, new_state->history->intervals: %p",
           new_state, new_state->history, new_state->history->intervals);
    delete_state(new_state, true);

    logger(LOG_LEVEL_DEBUG,
           "expected_state: %p, expected_state->history: %p, expected_state->history->intervals: %p",
           expected_state, expected_state->history, expected_state->history->intervals);
    delete_state(expected_state, true);

    logger(LOG_LEVEL_DEBUG,
           "another_new_state: %p, another_new_state->history: %p, another_new_state->history->intervals: %p",
           another_new_state, another_new_state->history, another_new_state->history->intervals);
    delete_state(another_new_state, true);

    delete_heartbeat_history(initial_history);
    delete_heartbeat_history(new_history);
    delete_heartbeat_history(expected_history);
    delete_heartbeat_history(another_new_history);
}

void test_history_and_state_malloc_and_free(void) {
    heartbeat_history_t *history = new_heartbeat_history(
            5, NULL, 0, 0.f, 0.f);
    logger(LOG_LEVEL_INFO2, "history: %p, history->intervals: %p", history, history->intervals);
    state_t *state = state_init(history, 1000);
    logger(LOG_LEVEL_INFO2, "state: %p", state);

    // Clean up
    delete_state(state, true);  // this is because state_init is creating a new allocated history, so It
    // does not match the history created above, and we got 2 pointers to different memory locations:
    // state->history and history
    delete_heartbeat_history(history);
}


void test_heartbeat(void) {
#undef DEBUG_TIMES
    fake_time = 1000 * 1000;
    float threshold = 1.5f;
    size_t max_sample_size = 5;
    float min_std_deviation_ms = 0.1f;
    float acceptable_heartbeat_pause_ms = 1.0f;
    float first_heartbeat_estimate_ms = 0.5f;

    // Create a phi accrual detector
    phi_accrual_detector *detector = new_phi_accrual_detector(
            threshold, max_sample_size, min_std_deviation_ms,
            acceptable_heartbeat_pause_ms, first_heartbeat_estimate_ms, NULL);

    // Initial heartbeat
    heartbeat(detector);
    TEST_ASSERT_EQUAL(1000, detector->state->timestamp);
    logger(LOG_LEVEL_INFO2, "detector->state->timestamp: %lld", detector->state->timestamp);

    // Add another heartbeat at a later time
    fake_time = 2000 * 1000;
    heartbeat(detector);
    TEST_ASSERT_EQUAL(2000, detector->state->timestamp);

    delete_phi_accrual_detector(detector);
#define DEBUG_TIMES

}

int main(void) {
    logConfig config = {
            .show_timestamp = 1,
            .show_logger_name = 1,
            .log_to_console = 1,
            .log_level = LOG_LEVEL_ERROR
    };

    Logger_init("logger_name", &config, &test_logger);

    UNITY_BEGIN();
    RUN_TEST(test_new_phi_accrual_detector);
    RUN_TEST(test_is_available);
    RUN_TEST(test_get_phi);
    RUN_TEST(test_heartbeat);
    RUN_TEST(test_compare_and_set);
    RUN_TEST(test_ensure_valid_std_deviation);
    RUN_TEST(test_history_and_state_malloc_and_free);
    UNITY_END();

    check_for_leaks();
    return 0;
}