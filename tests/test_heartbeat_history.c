#include "unity.h"
#include "math.h"
#include "qos/accrual_detector/heartbeat_history.h"
#include "utils/memory_leak_detector.h"

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void test_new_history(void) {
    size_t max_sample_size = 1;
    heartbeat_history_t *history = new_heartbeat_history(max_sample_size, NULL, 0, 0.f, 0.f);

    // Verify that history is not NULL
    TEST_ASSERT_NOT_NULL(history);

    // Check that the fields are initialized correctly
    TEST_ASSERT_NOT_NULL(history->intervals);
    TEST_ASSERT_EQUAL_UINT(max_sample_size, history->max_sample_size);
    TEST_ASSERT_EQUAL_UINT(0, history->interval_count);
    TEST_ASSERT_EQUAL_FLOAT(0.f, history->interval_sum);
    TEST_ASSERT_EQUAL_FLOAT(0.f, history->squared_interval_sum);

    // Clean up and verify that no memory is leaked
    delete_heartbeat_history(history);
}

void test_mean(void) {
    size_t max_sample_size = 3;
    heartbeat_history_t *history = new_heartbeat_history(max_sample_size, NULL, 0, 0.f, 0.f);

    // Add intervals and verify mean
    add_interval(history, 1.f);
    add_interval(history, 2.f);
    add_interval(history, 3.f);

    float expected_mean = (1.f + 2.f + 3.f) / 3.f;
    TEST_ASSERT_EQUAL_FLOAT(expected_mean, mean(history));

    delete_heartbeat_history(history);
}

void test_variance(void) {
    size_t max_sample_size = 3;
    heartbeat_history_t *history = new_heartbeat_history(max_sample_size, NULL, 0, 0.f, 0.f);

    // Add intervals and verify variance
    add_interval(history, 1.f);
    add_interval(history, 2.f);
    add_interval(history, 3.f);

    float m = mean(history);
    float expected_variance = ((1.f - m) * (1.f - m) + (2.f - m) * (2.f - m) + (3.f - m) * (3.f - m)) / 3.f;
    TEST_ASSERT_EQUAL_FLOAT(expected_variance, variance(history));

    delete_heartbeat_history(history);
}

void test_std_dev(void) {
    size_t max_sample_size = 3;
    heartbeat_history_t *history = new_heartbeat_history(max_sample_size, NULL, 0, 0.f, 0.f);

    // Add intervals and verify standard deviation
    add_interval(history, 1.f);
    add_interval(history, 2.f);
    add_interval(history, 3.f);

    float expected_std_dev = sqrtf(variance(history));
    TEST_ASSERT_EQUAL_FLOAT(expected_std_dev, std_dev(history));

    delete_heartbeat_history(history);
}

void test_adjust_intervals(void) {
    size_t max_sample_size = 3;
    heartbeat_history_t *history = new_heartbeat_history(max_sample_size, NULL, 0, 0.f, 0.f);

    // Add intervals
    add_interval(history, 10.f);
    add_interval(history, 20.f);
    add_interval(history, 30.f);

    // Adjust intervals
    int missed_count = 2;
    adjust_intervals(history, missed_count);

    // Verify that intervals are adjusted correctly
    float scaling_factor = get_scaling_factor(missed_count);
    float expected_intervals[] = {10.f * scaling_factor, 20.f * scaling_factor, 30.f * scaling_factor};

    for (size_t i = 0; i < history->interval_count; ++i) {
        TEST_ASSERT_EQUAL_FLOAT(expected_intervals[i], history->intervals[i]);
    }

    delete_heartbeat_history(history);
}


void test_compare_history(void) {
    size_t max_sample_size = 3;
    heartbeat_history_t *history1 = new_heartbeat_history(max_sample_size, NULL, 0, 0.f, 0.f);
    heartbeat_history_t *history2 = new_heartbeat_history(max_sample_size, NULL, 0, 0.f, 0.f);

    // Add same intervals to both histories
    add_interval(history1, 10.f);
    add_interval(history2, 10.f);

    // Verify that histories are equal
    TEST_ASSERT_TRUE(compare_history(history1, history2));

    // Add different interval to history2
    add_interval(history2, 20.f);

    // Verify that histories are not equal
    TEST_ASSERT_FALSE(compare_history(history1, history2));

    delete_heartbeat_history(history1);
    delete_heartbeat_history(history2);
}

void test_add_interval(void) {
    size_t max_sample_size = 5;
    heartbeat_history_t *history = new_heartbeat_history(max_sample_size, NULL, 0, 0.f, 0.f);
    TEST_ASSERT_NOT_NULL(history);

    // Add intervals and check that they are added correctly
    for (size_t i = 0; i < max_sample_size; ++i) {
        add_interval(history, (float) (i + 1));
        TEST_ASSERT_EQUAL_UINT(i + 1, history->interval_count);
        TEST_ASSERT_EQUAL_FLOAT((float) (i + 1), history->intervals[i]);
    }

    // Clean up
    delete_heartbeat_history(history);
}

void test_drop_oldest_interval(void) {
    size_t max_sample_size = 5;
    heartbeat_history_t *history = new_heartbeat_history(max_sample_size, NULL, 0, 0.f, 0.f);
    TEST_ASSERT_NOT_NULL(history);

    // Fill the history to its max size
    for (size_t i = 0; i < max_sample_size; ++i) {
        add_interval(history, (float) (i + 1));
    }

    // Drop the oldest interval and check the result
    drop_oldest_interval(history);
    TEST_ASSERT_EQUAL_UINT(max_sample_size - 1, history->interval_count);
    for (size_t i = 0; i < history->interval_count; ++i) {
        TEST_ASSERT_EQUAL_FLOAT((float) (i + 2), history->intervals[i]); // Should have shifted left
    }

    // Clean up
    delete_heartbeat_history(history);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_new_history);
    RUN_TEST(test_add_interval);
    RUN_TEST(test_drop_oldest_interval);
    RUN_TEST(test_mean);
    RUN_TEST(test_variance);
    RUN_TEST(test_std_dev);
    RUN_TEST(test_adjust_intervals);
    RUN_TEST(test_compare_history);
    check_for_leaks();
    return UNITY_END();
}
