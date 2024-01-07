#include "heartbeat_history.h"
#include "utils/memory_leak_detector.h"

#define MAX(a, b) ((a > b) ? (a) : (b))


/**
 * Create a new heartbeat_history_t object
 * @param max_sample_size represents the maximum number of intervals that can be stored in the history (WINDOW_SIZE)
 * @param intervals is an array of intervals
 * @param interval_count is the number of intervals in the array
 * @param interval_sum is the sum of all intervals
 * @param squared_interval_sum is the sum of all squared intervals
 * @return a pointer to the new heartbeat_history_t object
 */
heartbeat_history_t *
new_heartbeat_history(
        size_t max_sample_size,
        float *intervals,
        size_t interval_count,
        float interval_sum,
        float squared_interval_sum) {

    heartbeat_history_t *history = malloc(sizeof(heartbeat_history_t));
    if (!history) {
        perror("Failed to allocate heartbeat_history_t");
        delete_heartbeat_history(history);
        return NULL;
    }

    // Intervals
    void *tmp;
    if (intervals == NULL) {
        tmp = calloc(max_sample_size, sizeof(float));
        if (tmp == NULL) {
            perror("Failed to allocate intervals");
            return NULL;
        }
        history->intervals = tmp;
        history->interval_count = 0;
    } else {
        history->intervals = intervals;
        history->interval_count = interval_count;
    }

    // Max sample size
    if (max_sample_size < 1) {
        perror("max_sample_size must be > 0");
        free(tmp);
        tmp = NULL;
        return NULL;
    }

    // Interval sum
    if (interval_sum < 0) {
        perror("interval_sum must be >= 0");
        free(tmp);
        tmp = NULL;
        return NULL;
    }

    // Squared interval sum
    if (squared_interval_sum < 0) {
        perror("squared_interval_sum must be >= 0");
        free(tmp);
        tmp = NULL;
        return NULL;
    }

    history->max_sample_size = max_sample_size;
    history->interval_sum = interval_sum;
    history->squared_interval_sum = squared_interval_sum;

    return history;
}

/**
 * Delete a heartbeat_history_t object
 * @param history is a pointer to the heartbeat_history_t object to delete
 */
void delete_heartbeat_history(heartbeat_history_t *history) {
    if (history == NULL) {
        return;
    }

    if (history->intervals != NULL) {
        free(history->intervals);
        history->intervals = NULL;
    }
    free(history);
    history = NULL;
}

/**
 * Calculate the mean of the intervals in the history
 * @param history is a pointer to the heartbeat_history_t object
 * @return the mean of the intervals in the history
 */
float mean(const heartbeat_history_t *history) {
    if (history->interval_count == 0) {
        return 0.f;
    }

    return history->interval_sum / (float) history->interval_count;
}

/**
 * Calculate the variance of the intervals in the history
 * @param history is a pointer to the heartbeat_history_t object
 * @return the variance of the intervals in the history
 */
float variance(const heartbeat_history_t *history) {
    if (history->interval_count == 0) {
        return 0.f;
    }

    float mean_val = mean(history);
    return (history->squared_interval_sum / (float) history->interval_count) - (mean_val * mean_val);
}

/**
 * Calculate the standard deviation of the intervals in the history
 * @param history is a pointer to the heartbeat_history_t object
 * @return the standard deviation of the intervals in the history
 */
float std_dev(const heartbeat_history_t *history) {
    return sqrtf(variance(history));
}

/**
 * Drop the oldest interval in the history
 * @param history is a pointer to the heartbeat_history_t object
 */
void drop_oldest_interval(heartbeat_history_t *history) {
    if (history->interval_count == 0) return;
    for (int i = 1; i < history->max_sample_size; ++i) {
        history->intervals[i - 1] = history->intervals[i];
    }
    history->interval_count--;
}

/**
 * Add an interval to the history
 * @param history is a pointer to the heartbeat_history_t object
 * @param interval is the interval to add
 */
void add_interval(heartbeat_history_t *history, float interval) {
    if (history->interval_count < history->max_sample_size) {
        history->intervals[history->interval_count++] = interval;
    } else {
        drop_oldest_interval(history);
        history->intervals[history->max_sample_size - 1] = interval;
    }

    history->interval_sum = 0.f;
    history->squared_interval_sum = 0.f;
    for (int i = 0; i < history->interval_count; ++i) {
        history->interval_sum += history->intervals[i];
        history->squared_interval_sum += history->intervals[i] * history->intervals[i];
    }
}

/**
 * Calculate the scaling factor for the intervals in the history
 * @param missed_count is the number of missed heartbeats
 * @return the scaling factor for the intervals in the history
 */
float get_scaling_factor(int missed_count) {
    if (missed_count == 0) {
        missed_count = -1;
    }

    float base_scale = 1.0f;  // Normal scale for no missed heartbeats
    float scale_increment = 0.05f;  // Increment per missed heartbeat_history
    return base_scale - ((float) missed_count * scale_increment);
}

/**
 * Adjust the intervals in the history
 * @param history is a pointer to the heartbeat_history_t object
 * @param missed_count is the number of missed heartbeats
 */
void adjust_intervals(heartbeat_history_t *history, int missed_count) {
    float scaling_factor = get_scaling_factor(missed_count);
    for (int i = 0; i < history->interval_count; ++i) {
        history->intervals[i] = MAX(history->intervals[i] * scaling_factor, 0);
    }
    history->interval_sum = 0.0f;
    history->squared_interval_sum = 0.0f;
    for (int i = 0; i < history->interval_count; ++i) {
        history->interval_sum += history->intervals[i];
        history->squared_interval_sum += history->intervals[i] * history->intervals[i];
    }
}

/**
 * Compare two heartbeat_history_t objects
 * @param history1 is a pointer to the first heartbeat_history_t object
 * @param history2 is a pointer to the second heartbeat_history_t object
 * @return true if the two heartbeat_history_t objects are equal, false otherwise
 */
bool compare_history(heartbeat_history_t *history1, heartbeat_history_t *history2) {
    if (history1 == NULL || history2 == NULL) return false;

    if (history1->max_sample_size != history2->max_sample_size) return false;
    if (history1->interval_count != history2->interval_count) return false;
    if (history1->interval_sum != history2->interval_sum) return false;
    if (history1->squared_interval_sum != history2->squared_interval_sum) return false;
    if (history1->intervals == NULL && history2->intervals == NULL) return true;
    if (history1->intervals == NULL || history2->intervals == NULL) return false;
    for (int i = 0; i < history1->interval_count; ++i) {
        if (history1->intervals[i] != history2->intervals[i]) return false;
    }
    return true;
}

/**
 * Update the first heartbeat_history_t object with the values from the second heartbeat_history_t object
 * @param first is a pointer to the first heartbeat_history_t object
 * @param second is a pointer to the second heartbeat_history_t object
 * @return
 */
void update_history(heartbeat_history_t *first, heartbeat_history_t *second) {
    if (first == NULL || second == NULL) return;

    // update number of intervals (malloc if necessary)
    if (first->max_sample_size != second->max_sample_size || first->intervals == NULL) {
        if (first->intervals != NULL) {
            free(first->intervals);
            first->intervals = NULL;
        }
        first->max_sample_size = second->max_sample_size;
        first->intervals = calloc(first->max_sample_size, sizeof(float));
    }
    first->interval_count = second->interval_count;
    first->interval_sum = second->interval_sum;
    first->squared_interval_sum = second->squared_interval_sum;

    // update intervals
    for (int i = 0; i < first->interval_count; ++i) {
        first->intervals[i] = second->intervals[i];
    }
}



